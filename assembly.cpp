#include <algorithm>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stack>
#include <string>
#include <type_traits>
#include <vector>

#include "declarations.hpp"
#include "tables.hpp"
#include "utility.hpp"

// #define DIAG
#include "diag.hpp"


static constexpr byte Unmapped = byte(-1);
typedef byte TablesMapping[TABLES_MAX_COUNT];


using namespace std;

template<class InputIt>
ifstream::off_type find_file_offset(const RvaAndSize& dst, InputIt first, InputIt last) {
  auto found = find_if(first, last, [&dst] (auto& entry) { 
    return entry.rva <= dst.rva && dst.rva + dst.sz < entry.rva + entry.sz_virt; });

  return found != last ? 
    found->file_offset + dst.rva - found->rva
      : -1;
}

template<class InputIt>
bool try_find_file_offset(const RvaAndSize& dst, InputIt first, InputIt last, ifstream::off_type& out_offset) {
  auto found = find_if(first, last, [&dst] (auto& entry) { 
    return entry.rva <= dst.rva && dst.rva + dst.sz < entry.rva + entry.sz_virt; });

  if (found != last) {
    out_offset = found->file_offset + dst.rva - found->rva;
    return true;
  }

  return false;
}


inline dword get_table_size(const TablesMapping& mapping, const dword size[], TableFlag table) {
  return size[mapping[static_cast<size_t>(table)]];
}

inline size_t get_index_size_h(const MetadataHeader& hdr, HeapSizesFlags heap) {
  return sizeof(word) << has_flag(hdr.heap_sizes, heap);
}

#define TABLE_INDEX_FIELD_SIZE_ESTIMATE(rowsCount, shift) \
  (sizeof(word) << ((rowsCount) >= ((dword(1) << (bitsizeof_(word) - (shift))) - 1)))
inline size_t get_index_size_t(const TablesMapping& mapping, const dword size[], TableFlag table, int shift = 0) {
  auto m = mapping[as_integral(table)];
  return TABLE_INDEX_FIELD_SIZE_ESTIMATE(m != Unmapped ? size[m] : 0, shift);
}

template<class TCol>
struct coded_index {

  static constexpr dword mask = ~(dword(-1) << TCol::shift);

  static constexpr dword decode(dword value, TableFlag& out_flag) {
    out_flag = TCol::m[(value & mask)];
    return (value >> TCol::shift) - 1;
  }

  static constexpr size_t get_size(const TablesMapping& mapping, const dword size[]) {
    size_t result = 0;

    for (auto t : TCol::m) {
      if (t != TableFlag::Undefined) {
        result = max(result, get_index_size_t(mapping, size, t, TCol::shift));
      }
    }

    return result;
  }

  coded_index() = delete;
};


static void get_string(istream& src, ifstream::off_type base, dword index, string& output) {
  auto ofs = src.tellg();

  src.seekg(base + index, istream::beg);
  getline(src, output, '\0');
  src.seekg(ofs, istream::beg);
}

static void get_guid(istream& src, ifstream::off_type base, dword index, guid& output) {
  auto ofs = src.tellg();

  src.seekg(base + (index - 1) * sizeof(guid), istream::beg);
  src.read(reinterpret_cast<char*>(&output), sizeof(guid));
  src.seekg(ofs, istream::beg);
}


class matcher {
public:
  virtual ~matcher() {}

  virtual bool operator()(const string& s) const = 0;
};

struct matcher_any : public matcher {
  bool operator()(const string& s) const override {
    return true;
  }
};

struct matcher_text : public matcher {
  regex _re;

  matcher_text(const string& text)
    : _re(text) { }

  bool operator()(const string& s) const override {
    return regex_match(s, _re);
  }
};


#include "optionparser.h"

struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    fprintf(stderr, "ERROR: %s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s", msg2);
  }
  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }
  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = 0;
    if (option.arg != 0 && strtol(option.arg, &endptr, 10)){};
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;
    if (msg) printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }
};

enum  optionIndex { UNKNOWN, HELP, OUT_GROUP, RE_ASM };
const option::Descriptor usage[] =
{
 {UNKNOWN,   0, ""  , ""        ,option::Arg::None,  "USAGE: assembly [options] -- <path/to/assembly.dll>\n\n"
                                                     "Options:" },
 {HELP,      0, ""  , "help"    , option::Arg::None, "  --help         \tPrint usage and exit." },
 {OUT_GROUP, 0, "g" , "group"   , option::Arg::None, "  --group, -g    \tThe resulting JSON is grouped by assembly name." },
 {RE_ASM,    0, "a" , "assembly", Arg::Required,     "  --assembly, -a \tAssemblies filter regexp." },

 {0,0,0,0,0,0}
};


int main(int argc, const char *argv[]) {
  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present

  option::Stats  stats(usage, argc, argv);
  option::Option options[stats.options_max], buffer[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error()) {
    return 1;
  }

  if (options[HELP] || argc == 0) {
    option::printUsage(cout, usage);
    return 0;
  }

  vector<unique_ptr<matcher>> matchers;
  {
    auto opt = (option::Option*)options[RE_ASM];

    while (opt) {
      try {
        matchers.push_back(make_unique<matcher_text>(opt->arg));
      }
      catch (regex_error&) {
        cerr << "'" << opt->arg << "' regex is ill-formed." << endl;
        return -1;
      }

      opt = opt->next();
    }

    if (!matchers.size()) {
      matchers.push_back(make_unique<matcher_any>());
    }
  }

  ifstream assembly;
  {
    if (parse.nonOptionsCount() != 1) {
      cerr << "Assembly file not specified" << endl;
      option::printUsage(cerr, usage);
      return 2;    
    }

    auto filePath = parse.nonOption(0);

    assembly.open(filePath, ifstream::binary);
    if (!assembly)
    {
      cerr << "Cannot open '" << filePath << "'" << endl;
      return -2;
    }
  }

  cout << boolalpha;

  HDR_MSDOS hdrMsDos;
  assembly.read(reinterpret_cast<char*>(&hdrMsDos), sizeof(hdrMsDos));
  assembly.seekg(hdrMsDos.e_lfanew, assembly.beg);

  HDR_COFF hdrCoff;
  assembly.read(reinterpret_cast<char*>(&hdrCoff), sizeof(hdrCoff));
  assembly.seekg(sizeof(HDR_COFF_STD), assembly.cur);

  HDR_COFF_WIN hdrCoffWin;
  assembly.read(reinterpret_cast<char*>(&hdrCoffWin), sizeof(hdrCoffWin));

  DIAGNOSTICS(
    cout << "Image Base:" << endl;
    cout << "  " << hdrCoffWin.image_base << endl;
    cout << endl;
  );

  vector<DataDirsEntry> dataDirs(hdrCoffWin.num_data_dirs);
  for (auto& entry : dataDirs) {
    assembly.read(reinterpret_cast<char*>(&entry), sizeof(entry));
  }

  auto& hdrCliEntry = dataDirs[14];

  DIAGNOSTICS(
    cout << "CLI header meta:" << endl;
    cout << "  size: " << hdrCliEntry.sz  << endl;
    cout << "  rva:  " << hdrCliEntry.rva << endl;
    cout << endl;
  );

  vector<SectionHeadersEntry> sectionHeaders(hdrCoff.num_sections);
  {
    for (auto& entry : sectionHeaders) {
      assembly.read(reinterpret_cast<char*>(&entry), sizeof(entry));

      DIAGNOSTICS(
        cout << entry.name << " section:" << endl;
        cout << "  V. size:  " << entry.sz_virt  << endl;
        cout << "  V. addr:  " << entry.rva << endl;
        cout << "  Raw size: " << entry.sz_raw   << endl;
        cout << "  Raw addr: " << entry.file_offset  << endl;
        cout << endl;
      );
    }
  }

  auto hdrCliHeaderOfs = find_file_offset(hdrCliEntry,
    sectionHeaders.begin(), sectionHeaders.end());

  assert(hdrCliHeaderOfs >= 0);
  assembly.seekg(hdrCliHeaderOfs, assembly.beg);

  HDR_CLI hdrCli;
  assembly.read(reinterpret_cast<char*>(&hdrCli), sizeof(hdrCli));

  DIAGNOSTICS(
    cout << "CLI header:" << endl;
    cout << "  size: " << hdrCli.sz << endl;
    cout << "  ver.: " << hdrCli.rt_major << "." << hdrCli.rt_minor << endl;
    cout << endl;
  );

  auto rootMetaOfs = find_file_offset(hdrCli.meta,
    sectionHeaders.begin(), sectionHeaders.end());

  assert(rootMetaOfs >= 0);
  assembly.seekg(rootMetaOfs, assembly.beg);

  MetadataRoot rootMeta;
  assembly.read(reinterpret_cast<char*>(&rootMeta), sizeof(rootMeta));

  string cliVersion(rootMeta.sz_version, 0);
  assembly.read(&cliVersion[0], cliVersion.size());
  assembly.seekg(round_up(4, rootMeta.sz_version)
    + sizeof(word) - rootMeta.sz_version, assembly.cur);

  map<string, StreamHeader> streamHeaders;
  {
    word numStreams;
    assembly.read(reinterpret_cast<char*>(&numStreams), sizeof(numStreams));

    DIAGNOSTICS(
      cout << "Metadata root [0x" << hex
        << rootMetaOfs << dec << "]:" << endl;
      cout << "  signature:     " << hex << showbase 
        << rootMeta.sig << dec << endl;
      cout << "  version:       " << cliVersion << endl;
      cout << "  streams count: " << numStreams << endl;
      cout << endl;
    );

    StreamHeader entry;
    string streamName;
    for (; numStreams > 0; --numStreams) {
      assembly.read(reinterpret_cast<char*>(&entry), sizeof(entry));
      getline(assembly, streamName, '\0');
      ignore_all(assembly, 4);

      streamHeaders[streamName] = entry;

      DIAGNOSTICS(
        cout << streamName << " stream:" << endl;
        cout << "  size:   " << entry.sz << endl;
        cout << "  offset: " << entry.ofs << endl;
        cout << endl;
      );
    }
  }

  auto& streamHdrTilda = streamHeaders["#~"];

  MetadataHeader hdrMeta;
  {
    auto tildaHeaderOfs = rootMetaOfs + streamHdrTilda.ofs;

    assembly.seekg(tildaHeaderOfs, assembly.beg);
    assembly.read(reinterpret_cast<char*>(&hdrMeta), sizeof(hdrMeta));

    DIAGNOSTICS(
      cout << "Metadata header:" << endl;
      cout << "  version:         " << static_cast<int>(hdrMeta.ver_major) << "."
        << static_cast<int>(hdrMeta.ver_minor) << endl;
      cout << "  heap size flags: " << hex << showbase << static_cast<int>(hdrMeta.heap_sizes)
        << dec << endl;
      cout << "  reserve:         " << static_cast<int>(hdrMeta.reserved) << endl;
      cout << "  valid:           " << bitset<64>(hdrMeta.valid) << endl;
      cout << "  sorted:          " << bitset<64>(hdrMeta.sorted) << endl;
      cout << "  has TypeRef?:    " << is_bit_set(hdrMeta.valid,
        as_integral(TableFlag::TypeRef)) << endl;
      cout << endl;
    );
  }

  TablesMapping tablesMapping;
  fill(tablesMapping, tablesMapping + countof_(tablesMapping), Unmapped);

  dword tableSizes[ones(hdrMeta.valid)];
  assembly.read(reinterpret_cast<char*>(&tableSizes), sizeof(tableSizes));
  {
    auto ctl = hdrMeta.valid;
    for(size_t i = 0, j = 0; ctl; ctl >>= 1, ++j) {
      if (ctl & 1) {
        tablesMapping[j] = i++;
      }
    }
  }

  IndexSize indexSize = {
    { // heap
      get_index_size_h(hdrMeta, HeapSizesFlags::Blob),
      get_index_size_h(hdrMeta, HeapSizesFlags::Guid),
      get_index_size_h(hdrMeta, HeapSizesFlags::String),
    },
    { // coded_cols
      coded_index<CustomAttributeType> ::get_size(tablesMapping, tableSizes),
      coded_index<HasConstant>         ::get_size(tablesMapping, tableSizes),
      coded_index<HasCustomAttribute>  ::get_size(tablesMapping, tableSizes),
      coded_index<HasDeclSecurity>     ::get_size(tablesMapping, tableSizes),
      coded_index<HasFieldMarshal>     ::get_size(tablesMapping, tableSizes),
      coded_index<HasSemantics>        ::get_size(tablesMapping, tableSizes),
      coded_index<Implementation>      ::get_size(tablesMapping, tableSizes),
      coded_index<MemberForwarded>     ::get_size(tablesMapping, tableSizes),
      coded_index<MemberRefParent>     ::get_size(tablesMapping, tableSizes),
      coded_index<MethodDefOrRef>      ::get_size(tablesMapping, tableSizes),
      coded_index<ResolutionScope>     ::get_size(tablesMapping, tableSizes),
      coded_index<TypeDefOrRef>        ::get_size(tablesMapping, tableSizes),
      coded_index<TypeOrMethodDef>     ::get_size(tablesMapping, tableSizes),
    },
  };
  
  for (size_t i = 0; i < countof_(tablesMapping); ++i) {
    auto t = tablesMapping[i];
    indexSize.plain_cols.m[i] = TABLE_INDEX_FIELD_SIZE_ESTIMATE(
      t != Unmapped ? tableSizes[t] : 0, 0);
  }

  DIAGNOSTICS(
    stringstream tables;
    tables << "table (" << ones(hdrMeta.valid) << ")";
    cout << setw(25) << left  << tables.str();
    cout << setw(8)  << right << "rows";
    cout << setw(8)  << right << "size";
    cout << setw(10) << right << "offset";
    cout << endl;
  );

  const ifstream::off_type tablesOffset = assembly.tellg();
  ifstream::off_type tableOffsets[ones(hdrMeta.valid)];
  {
    auto cumulativeOffset = tablesOffset;
    for (size_t i = 0; i < countof_(tablesMapping); ++i) {
      auto t = tablesMapping[i];

      if (t != Unmapped) {
        const auto tableRowsCount = tableSizes[t];
        const auto tableFlag = TableFlag(i);
        const auto tableSize = get_table_row_size(indexSize, tableFlag);

        tableOffsets[t] = cumulativeOffset;

        DIAGNOSTICS(
          cout << setfill('.') << setw(25) << left << tableFlag;
          cout << setw(8) << internal << tableRowsCount;
          cout << setw(8) << internal << tableSize;
          cout << setw(10) << setfill('\0') << right
            << hex << showbase << cumulativeOffset << dec;
          cout << endl;
        );

        cumulativeOffset += tableRowsCount * tableSize;
      }
    }
  }

  DIAGNOSTICS(
    cout << endl;
  );

  auto& streamHdrStrings = streamHeaders["#Strings"];
  auto& streamHdrGuid    = streamHeaders["#GUID"];

  string s;

  vector<pair<string, string>> results;
  {
    auto m = tablesMapping[as_integral(TableFlag::TypeRef)];
    size_t typeRefsCount = tableSizes[m];

    struct call_context {
      ifstream::off_type offset;
      TableFlag table;
    };

    stack<call_context> call_stack;
    vector<string> name_parts;
    TableFlag table_decoded = TableFlag::MemberRef;

    call_stack.push({
      tableOffsets[m],
      TableFlag::TypeRef,
    });

    while (call_stack.size() > 0) {
      auto& ctx = call_stack.top();

      assembly.seekg(ctx.offset, assembly.beg);

      switch (ctx.table) {
        case TableFlag::Module:
        case TableFlag::ModuleRef:
          // TODO: both are skipped for now.
          name_parts.clear();
          break;
        case TableFlag::AssemblyRef: {
          AssemblyRefTable table;
          {
            AssemblyRefTable::meta meta(indexSize);

            char data[meta.row_size()];
            assembly.read(data, sizeof(data));
            meta.from_bytes(data, table);
          }

          if (name_parts.size()) {
            string type_name = name_parts.back();
            name_parts.pop_back();

            for (auto it = name_parts.rbegin();
                it != name_parts.rend();) {
              type_name = *it++ + "." + type_name;
            }

            name_parts.clear();

            get_string(assembly, rootMetaOfs + streamHdrStrings.ofs, table.name, s);
            for (auto& m : matchers) {
              if ((*m)(s)) {
                results.push_back(make_pair(s, type_name));
                break;
              }
            }
          }
          break;
        }
        case TableFlag::TypeRef: {
          TypeRefTable table;
          {
            TypeRefTable::meta meta(indexSize);

            char data[meta.row_size()];
            assembly.read(data, sizeof(data));
            meta.from_bytes(data, table);

            if (call_stack.size() == 1
                && --typeRefsCount) {
              ctx.offset += meta.row_size();
            } else {
              call_stack.pop();
            }
          }

          if (table.type_namespace != 0) {
            get_string(assembly, rootMetaOfs + streamHdrStrings.ofs, table.type_namespace, s);
            name_parts.push_back(s);
          }

          get_string(assembly, rootMetaOfs + streamHdrStrings.ofs, table.type_name, s);
          name_parts.push_back(s);   

          auto idx = coded_index<ResolutionScope>::decode(
            table.resolution_scope, table_decoded);

          call_stack.push({ 
            ifstream::off_type(tableOffsets[tablesMapping[as_integral(table_decoded)]] + idx
              * get_table_row_size(indexSize, table_decoded)),
            table_decoded
          });

          continue;
        }
      }

      call_stack.pop();
    }
  }

  cout << "[";
  if (results.size()) {
    auto sep = "";
    if (options[OUT_GROUP] != nullptr) {
      map<string, vector<string>> grouping;
      for(auto& p : results) {
        grouping[p.first].push_back(p.second);
      }

      for(auto& g : grouping) {
        cout << sep;
        cout << "{"
             << "\"assembly\":\"" << g.first << "\","
             << "\"types\":[";

        auto sep1 = "";
        for (auto& s : g.second) {
          cout << sep1;
          cout << "\"" << s << "\"";

          sep1 = ",";
        }

        cout << "]}";

        sep = ",";
      }
    }
    else {
      for(auto& p : results) {
        cout << sep;
        cout << "{"
             << "\"assembly\":\"" << p.first  << "\","
             << "\"type\":\""     << p.second << "\""
             << "}";\

        sep = ",";
      }
    }
  }

  cout << "]" << endl;

  return 0;
}
