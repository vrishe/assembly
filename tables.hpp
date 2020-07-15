#pragma once

#ifndef TABLES_HPP_
#define TABLES_HPP_

#include "declarations.hpp"
#include "utility.hpp"

#define CODED_COLS_COUNT 13
#define TABLES_MAX_COUNT (sizeof(MetadataHeader::valid) << 3)

struct IndexSize {

  template<class T>
  struct get_val {
    static dword f(const char* (&d)) {
      auto result = *reinterpret_cast<T*>(const_cast<char*>(d));
      d += sizeof(T);
      return result;
    }
  };

  struct {
    size_t blob;
    size_t guid;
    size_t string;
    // size_t user_strings;

    inline dword get_idx_blob(const char* (&d)) const {
      return f_[(blob >> 1) - 1](d);
    }

    inline dword get_idx_guid(const char* (&d)) const {
      return f_[(guid >> 1) - 1](d);
    }

    inline dword get_idx_string(const char* (&d)) const {
      return f_[(string >> 1) - 1](d);
    }
  } heap;

  struct {
    size_t m[CODED_COLS_COUNT];

    size_t operator[](size_t id) const {
      return m[id];
    } 

    inline dword get_idx_coded(const char* (&d), size_t id) const {
      return f_[(m[id] >> 1) - 1](d);
    }
  } coded_cols;

  struct {

    size_t m[TABLES_MAX_COUNT];

    size_t operator[](TableFlag table) const {
      return m[as_integral(table)];
    }

    size_t& operator[](TableFlag table) {
      return m[as_integral(table)];
    }

    inline dword get_idx_plain(const char* (&d), TableFlag t) const {
      return f_[(m[static_cast<size_t>(t)] >> 1) - 1](d);
    }
  } plain_cols;

private:

  typedef dword (*get_idx_f)(const char* (&d));
  static constexpr get_idx_f f_[2] = {
    get_val< word>::f,
    get_val<dword>::f,
  };
};

struct TableMeta_ {
protected:
    TableMeta_(const IndexSize& hs)
      : _hs(&hs) {}

    const IndexSize* const _hs; 
};

struct ModuleTable {
  static constexpr TableFlag id = TableFlag::Module;

  word  generation;
  dword name;
  dword id_module_version;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, ModuleTable& dst) const {
      dst.generation = IndexSize::get_val<word>::f(src);
      dst.name = _hs->heap.get_idx_string(src);
      dst.id_module_version = _hs->heap.get_idx_guid(src);
    }

    size_t row_size() const {
      return sizeof(word) + _hs->heap.string + 3*_hs->heap.guid;
    }
  };
};

struct TypeRefTable {
  static constexpr TableFlag id = TableFlag::TypeRef;

  dword resolution_scope;
  dword type_name;
  dword type_namespace;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, TypeRefTable& dst) const {
      dst.resolution_scope = _hs->coded_cols.get_idx_coded(src, ResolutionScope::id);
      dst.type_name = _hs->heap.get_idx_string(src);
      dst.type_namespace = _hs->heap.get_idx_string(src);
    }

    size_t row_size() const {
      return _hs->coded_cols[ResolutionScope::id] + 2*_hs->heap.string;
    }
  }; 
};

struct TypeDefTable {
  static constexpr TableFlag id = TableFlag::TypeDef;

  dword flags;
  dword type_name;
  dword type_namespace;
  dword extends;
  dword field_list;
  dword method_list;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, TypeDefTable& dst) const {
      dst.flags = IndexSize::get_val<dword>::f(src);
      dst.type_name = _hs->heap.get_idx_string(src);
      dst.type_namespace = _hs->heap.get_idx_string(src);
      dst.extends = _hs->coded_cols.get_idx_coded(src, TypeDefOrRef::id);
      dst.field_list = _hs->plain_cols.get_idx_plain(src, TableFlag::Field);
      dst.method_list = _hs->plain_cols.get_idx_plain(src, TableFlag::MethodDef);
    }

    size_t row_size() const {
      return sizeof(dword)
        + 2 * _hs->heap.string
        + _hs->coded_cols[TypeDefOrRef::id]
        + _hs->plain_cols[TableFlag::Field]
        + _hs->plain_cols[TableFlag::MethodDef];
    }
  }; 
};

struct FieldTable {
  static constexpr TableFlag id = TableFlag::Field;

  word  flags;
  dword name;
  dword signature;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, FieldTable& dst) const {
      dst.flags = IndexSize::get_val<word>::f(src);
      dst.name = _hs->heap.get_idx_string(src);
      dst.signature = _hs->heap.get_idx_blob(src);
    }

    size_t row_size() const {
      return sizeof(word)
        + _hs->heap.string
        + _hs->heap.blob;
    }
  }; 
};

struct MethodDefTable {
  static constexpr TableFlag id = TableFlag::MethodDef;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(dword)
        + 2 * sizeof(word)
        + _hs->heap.string
        + _hs->heap.blob
        + _hs->plain_cols[TableFlag::Param];
    }
  }; 
};

struct ParamTable {
  static constexpr TableFlag id = TableFlag::Param;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 2 * sizeof(word)
        + _hs->heap.string;
    }
  }; 
};

struct InterfaceImplTable {
  static constexpr TableFlag id = TableFlag::InterfaceImpl;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->plain_cols[TableFlag::Param]
        + _hs->coded_cols[TypeDefOrRef::id];
    }
  };  
};

struct MemberRefTable {
  static constexpr TableFlag id = TableFlag::MemberRef;

  dword cls;
  dword name;
  dword signature;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, MemberRefTable& dst) const {
      dst.cls = _hs->coded_cols.get_idx_coded(src, MemberRefParent::id);
      dst.name = _hs->heap.get_idx_string(src);
      dst.signature = _hs->heap.get_idx_blob(src);
    }

    size_t row_size() const {
      return _hs->coded_cols[MemberRefParent::id]
        + _hs->heap.string
        + _hs->heap.blob;
    }
  };  
};

struct ConstantTable {
  static constexpr TableFlag id = TableFlag::Constant;

  byte  type;   // 0x02 - 0x0e, 0x12
  dword parent;
  dword value;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, ConstantTable& dst) const {
      dst.type = static_cast<byte>(IndexSize::get_val<word>::f(src));
      dst.parent = _hs->coded_cols.get_idx_coded(src, HasConstant::id);
      dst.value = _hs->heap.get_idx_blob(src);
    }

    size_t row_size() const {
      return 2*sizeof(byte)
        + _hs->coded_cols[HasConstant::id]
        + _hs->heap.blob;
    }
  };  
};

struct CustomAttributeTable {
  static constexpr TableFlag id = TableFlag::CustomAttribute;

  dword parent;
  dword type;
  dword value;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, CustomAttributeTable& dst) const {
      dst.parent = _hs->coded_cols.get_idx_coded(src, HasCustomAttribute::id);
      dst.type = _hs->coded_cols.get_idx_coded(src, CustomAttributeType::id);
      dst.value = _hs->heap.get_idx_blob(src);
    }

    size_t row_size() const {
      return _hs->coded_cols[HasCustomAttribute::id]
        + _hs->coded_cols[CustomAttributeType::id]
        + _hs->heap.blob;
    }
  };  
};

struct FieldMarshalTable {
  static constexpr TableFlag id = TableFlag::FieldMarshal;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->coded_cols[HasFieldMarshal::id]
        + _hs->heap.blob;
    }
  };
};

struct DeclSecurityTable {
  static constexpr TableFlag id = TableFlag::DeclSecurity;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + _hs->coded_cols[HasDeclSecurity::id]
        + _hs->heap.blob;
    }
  };    
};

struct ClassLayoutTable {
  static constexpr TableFlag id = TableFlag::ClassLayout;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + sizeof(dword)
        + _hs->plain_cols[TableFlag::TypeDef];
    }
  };  
};

struct FieldLayoutTable {
  static constexpr TableFlag id = TableFlag::FieldLayout;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(dword)
        + _hs->plain_cols[TableFlag::Field];
    }
  }; 
};

struct StandAloneSigTable {
  static constexpr TableFlag id = TableFlag::StandAloneSig;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->heap.string;
    }
  }; 
};

struct EventMapTable {
  static constexpr TableFlag id = TableFlag::EventMap;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->plain_cols[TableFlag::TypeDef]
        + _hs->plain_cols[TableFlag::Event];
    }
  }; 
};

struct EventTable {
  static constexpr TableFlag id = TableFlag::Event;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + _hs->heap.string
        + _hs->coded_cols[TypeDefOrRef::id];
    }
  }; 
};

struct PropertyMapTable {
  static constexpr TableFlag id = TableFlag::PropertyMap;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->plain_cols[TableFlag::TypeDef]
        + _hs->plain_cols[TableFlag::Property];
    }
  }; 
};

struct PropertyTable {
  static constexpr TableFlag id = TableFlag::Property;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + _hs->heap.string
        + _hs->heap.blob;
    }
  }; 
};

struct MethodSemanticsTable {
  static constexpr TableFlag id = TableFlag::MethodSemantics;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + _hs->plain_cols[TableFlag::MethodDef]
        + _hs->coded_cols[HasSemantics::id];
    }
  }; 
};

struct MethodImplTable {
  static constexpr TableFlag id = TableFlag::MethodImpl;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->plain_cols[TableFlag::TypeDef]
        + 2 * _hs->coded_cols[MethodDefOrRef::id];
    }
  }; 
};

struct ModuleRefTable {
  static constexpr TableFlag id = TableFlag::ModuleRef;

  dword name;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, ModuleRefTable& dst) const {
      dst.name = _hs->heap.get_idx_string(src);
    }

    size_t row_size() const {
      return _hs->heap.string;
    }
  }; 
};

struct TypeSpecTable {
  static constexpr TableFlag id = TableFlag::TypeSpec;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->heap.blob;
    }
  }; 
};

struct ImplMapTable {
  static constexpr TableFlag id = TableFlag::ImplMap;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + _hs->coded_cols[MemberForwarded::id]
        + _hs->heap.string
        + _hs->plain_cols[TableFlag::ModuleRef];
    }
  }; 
};

struct FieldRVATable {
  static constexpr TableFlag id = TableFlag::FieldRVA;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(dword)
        + _hs->plain_cols[TableFlag::Field];
    }
  }; 
};

struct AssemblyTable {
  static constexpr TableFlag id = TableFlag::Assembly;

  dword hash_alg_id;
  word  ver_major, ver_minor;
  word  num_build, num_revision;
  dword flags;
  dword public_key;
  dword name;
  dword culture;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, AssemblyTable& dst) const {
      dst.hash_alg_id = IndexSize::get_val<dword>::f(src);
      dst.ver_major = IndexSize::get_val<word>::f(src);
      dst.ver_minor = IndexSize::get_val<word>::f(src);
      dst.num_build = IndexSize::get_val<word>::f(src);
      dst.num_revision = IndexSize::get_val<word>::f(src);
      dst.flags = IndexSize::get_val<dword>::f(src);
      dst.public_key = _hs->heap.get_idx_blob(src);
      dst.name = _hs->heap.get_idx_string(src);
      dst.culture = _hs->heap.get_idx_string(src);
    }

    size_t row_size() const {
      return 2 * sizeof(dword)
        + 4 * sizeof(word)
        + _hs->heap.blob
        + 2 * _hs->heap.string;
    }
  }; 
};

struct AssemblyProcessorTable {
  static constexpr TableFlag id = TableFlag::AssemblyProcessor;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(dword);
    }
  }; 
};

struct AssemblyOSTable {
  static constexpr TableFlag id = TableFlag::AssemblyOS;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 3 * sizeof(dword);
    }
  }; 
};

struct AssemblyRefTable {
  static constexpr TableFlag id = TableFlag::AssemblyRef;

  word  ver_major, ver_minor;
  word  num_build, num_revision;
  dword flags;
  dword public_key_or_token;
  dword name;
  dword culture;
  dword hash_value;

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    void from_bytes(const char* src, AssemblyRefTable& dst) const {
      dst.ver_major = IndexSize::get_val<word>::f(src);
      dst.ver_minor = IndexSize::get_val<word>::f(src);
      dst.num_build = IndexSize::get_val<word>::f(src);
      dst.num_revision = IndexSize::get_val<word>::f(src);
      dst.flags = IndexSize::get_val<dword>::f(src);
      dst.public_key_or_token = _hs->heap.get_idx_blob(src);
      dst.name = _hs->heap.get_idx_string(src);
      dst.culture = _hs->heap.get_idx_string(src);
      dst.hash_value = _hs->heap.get_idx_blob(src);
    }

    size_t row_size() const {
      return 4 * sizeof(word)
        + sizeof(dword)
        + 2 * _hs->heap.blob
        + 2 * _hs->heap.string;
    }
  }; 
};

struct AssemblyRefProcessorTable {
  static constexpr TableFlag id = TableFlag::AssemblyRefProcessor;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(dword)
        + _hs->plain_cols[TableFlag::AssemblyRef];
    }
  }; 
};

struct AssemblyRefOSTable {
  static constexpr TableFlag id = TableFlag::AssemblyRefOS;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 3 * sizeof(dword)
        + _hs->plain_cols[TableFlag::AssemblyRef];
    }
  }; 
};

struct FileTable {
  static constexpr TableFlag id = TableFlag::File;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return sizeof(word)
        + _hs->heap.string
        + _hs->heap.blob;
    }
  }; 
};

struct ExportedTypeTable {
  static constexpr TableFlag id = TableFlag::ExportedType;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 2 * sizeof(dword)
        + 2 * _hs->heap.string
        + _hs->coded_cols[Implementation::id];
    }
  }; 
};

struct ManifestResourceTable {
  static constexpr TableFlag id = TableFlag::ManifestResource;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 2 * sizeof(dword)
        + _hs->heap.string
        + _hs->coded_cols[Implementation::id];
    }
  }; 
};

struct NestedClassTable {
  static constexpr TableFlag id = TableFlag::NestedClass;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 2 * _hs->plain_cols[TableFlag::TypeDef];
    }
  }; 
};

struct GenericParamTable {
  static constexpr TableFlag id = TableFlag::GenericParam;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return 2 * sizeof(word)
        + _hs->coded_cols[TypeOrMethodDef::id]
        + _hs->heap.string;
    }
  }; 
};

struct MethodSpecTable {
  static constexpr TableFlag id = TableFlag::MethodSpec;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->coded_cols[TypeOrMethodDef::id]
        + _hs->heap.blob;
    }
  }; 
};

struct GenericParamConstraintTable {
  static constexpr TableFlag id = TableFlag::GenericParamConstraint;

  // TODO: fields

  struct meta : protected TableMeta_ {

    meta(const IndexSize& hs)
      : TableMeta_(hs) {}

    size_t row_size() const {
      return _hs->plain_cols[TableFlag::GenericParam]
        + _hs->coded_cols[TypeDefOrRef::id];
    }
  }; 
};

#define TABLE_SIZE_GETTER_(flag) \
case TableFlag::flag: return flag##Table::meta(hs).row_size()
static size_t get_table_row_size(const IndexSize& hs, TableFlag table) {
  switch (table) {
TABLE_SIZE_GETTER_(Assembly);
TABLE_SIZE_GETTER_(AssemblyOS);
TABLE_SIZE_GETTER_(AssemblyProcessor);
TABLE_SIZE_GETTER_(AssemblyRef);
TABLE_SIZE_GETTER_(AssemblyRefOS);
TABLE_SIZE_GETTER_(AssemblyRefProcessor);
TABLE_SIZE_GETTER_(ClassLayout);
TABLE_SIZE_GETTER_(Constant);
TABLE_SIZE_GETTER_(CustomAttribute);
TABLE_SIZE_GETTER_(DeclSecurity);
TABLE_SIZE_GETTER_(EventMap);
TABLE_SIZE_GETTER_(Event);
TABLE_SIZE_GETTER_(ExportedType);
TABLE_SIZE_GETTER_(Field);
TABLE_SIZE_GETTER_(FieldLayout);
TABLE_SIZE_GETTER_(FieldMarshal);
TABLE_SIZE_GETTER_(FieldRVA);
TABLE_SIZE_GETTER_(File);
TABLE_SIZE_GETTER_(GenericParam);
TABLE_SIZE_GETTER_(GenericParamConstraint);
TABLE_SIZE_GETTER_(ImplMap);
TABLE_SIZE_GETTER_(InterfaceImpl);
TABLE_SIZE_GETTER_(ManifestResource);
TABLE_SIZE_GETTER_(MemberRef);
TABLE_SIZE_GETTER_(MethodDef);
TABLE_SIZE_GETTER_(MethodImpl);
TABLE_SIZE_GETTER_(MethodSemantics);
TABLE_SIZE_GETTER_(MethodSpec);
TABLE_SIZE_GETTER_(Module);
TABLE_SIZE_GETTER_(ModuleRef);
TABLE_SIZE_GETTER_(NestedClass);
TABLE_SIZE_GETTER_(Param);
TABLE_SIZE_GETTER_(Property);
TABLE_SIZE_GETTER_(PropertyMap);
TABLE_SIZE_GETTER_(StandAloneSig);
TABLE_SIZE_GETTER_(TypeDef);
TABLE_SIZE_GETTER_(TypeRef);
TABLE_SIZE_GETTER_(TypeSpec);
  }

  return size_t(-1);
}

#endif // TABLES_HPP_
