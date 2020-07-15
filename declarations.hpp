#pragma once

#ifndef DECLARATIONS_HPP_
#define DECLARATIONS_HPP_

#include <cstdint>
#include "utility.hpp"


typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef byte guid[16];

#pragma pack(push, 1)

typedef struct {
  char  sig[2];         // MZ
  byte  skipped[58];
  dword e_lfanew;
} HDR_MSDOS;

typedef struct {
  char  sig[4];         // PE\0\0
  word  machine;
  word  num_sections;
  dword timestamp;
  dword skipped[2];
  word  sz_hdropt;
  word  flags;
} HDR_COFF;

typedef struct {
  word  magic;          // 010b
  word  skipped;
  dword sz_code;
  qword sz_data;
  dword rva_entry_point;
  dword base_of_code;
  dword base_of_data;
} HDR_COFF_STD;

typedef struct {
  dword image_base;
  dword sectioshift_alignment;
  dword file_alignment;
  word  os_major;
  word  os_minor;
  word  user_major;
  word  user_minor;
  word  subsystem_major;
  word  subsystem_minor;
  dword reserved;
  dword sz_image;
  dword sz_header;
  dword file_checksum;
  dword flags_dll;
  dword sz_stack_reserve;
  dword sz_stack_commit;
  dword sz_heap_reserve;
  dword sz_heap_commit;
  dword flags_loader;
  dword num_data_dirs;
} HDR_COFF_WIN;

struct RvaAndSize {
  dword rva;
  dword sz;
};

typedef struct {
  dword      sz;
  word       rt_major;
  word       rt_minor;
  RvaAndSize meta;
  dword      flags;
  dword      tokeshift_entry_pt;
  RvaAndSize resources;
  qword      rva_sshift_sig;
  qword      skipped_cmt; // Code Manager Table
  qword      rva_vt_fixups;
  qword      skipped[2];
} HDR_CLI;

typedef RvaAndSize DataDirsEntry;

struct SectionHeadersEntry {
  char  name[8];
  dword sz_virt;
  dword rva;
  dword sz_raw;
  dword file_offset;
  byte  skipped[12];
  dword characteristics;
};


// Metadata

struct MetadataRoot {
  dword sig;
  byte  skipped[8];
  dword sz_version;
};

struct StreamHeader {
  dword ofs;
  dword sz;
};

struct MetadataHeader {
  dword skipped;
  byte  ver_major;
  byte  ver_minor;
  byte  heap_sizes;
  byte  reserved; // Always 1
  qword valid;
  qword sorted;
};

#pragma pack(pop)


enum class HeapSizesFlags : byte {

/*

  Each signalize whether the stream has 4-byte indices 
  when a corresponding flag is set, 2-byte indices otherwise.

*/

  String = 0x01,
  Guid   = 0x02,
  Blob   = 0x04,
};

enum class TableFlag : unsigned short {
  Assembly = 0x20,
  AssemblyOS = 0x22,
  AssemblyProcessor = 0x21,
  AssemblyRef = 0x23,
  AssemblyRefOS = 0x25,
  AssemblyRefProcessor = 0x24,
  ClassLayout = 0x0F,
  Constant = 0x0B,
  CustomAttribute = 0x0C,
  DeclSecurity =0x0E,
  EventMap = 0x12,
  Event = 0x14,
  ExportedType = 0x27,
  Field = 0x04,
  FieldLayout = 0x10,
  FieldMarshal = 0x0D,
  FieldRVA = 0x1D,
  File = 0x26,
  GenericParam = 0x2A,
  GenericParamConstraint = 0x2C,
  ImplMap = 0x1C,
  InterfaceImpl = 0x09,
  ManifestResource = 0x28,
  MemberRef = 0x0A,
  MethodDef = 0x06,
  MethodImpl = 0x19,
  MethodSemantics = 0x18,
  MethodSpec = 0x2B,
  Module = 0x00,
  ModuleRef = 0x1A,
  NestedClass = 0x29,
  Param = 0x08,
  Property = 0x17,
  PropertyMap = 0x15,
  StandAloneSig = 0x11,
  TypeDef = 0x02,
  TypeRef = 0x01,
  TypeSpec = 0x1B,

  Undefined = 0xFF
};

struct CustomAttributeType {
  enum val_ {
    MethodDef = 2,
    MemberRef = 3,    
  };

  static constexpr TableFlag m[] = {
    TableFlag::Undefined,
    TableFlag::Undefined,
    TableFlag::MethodDef,
    TableFlag::MemberRef,
    TableFlag::Undefined,
  };

  static constexpr size_t id = 0;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  CustomAttributeType() = delete;
};

struct HasConstant {
  enum val_ {
    Field = 0,
    Param = 1,
    Property = 2,
  };

  static constexpr TableFlag m[] = {
    TableFlag::Field,
    TableFlag::Param,
    TableFlag::Property,
  };

  static constexpr size_t id = 1;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  HasConstant() = delete;
};

struct HasCustomAttribute {
  enum val_ {
    MethodDef = 0,
    Field = 1,
    TypeRef = 2,
    TypeDef = 3,
    Param = 4,
    InterfaceImpl = 5,
    MemberRef = 6,
    Module = 7,
    Permission = 8,
    Property = 9,
    Event = 10,
    StandAloneSig = 11,
    ModuleRef = 12,
    TypeSpec = 13,
    Assembly = 14,
    AssemblyRef = 15,
    File = 16,
    ExportedType = 17,
    ManifestResource = 18,
    GenericParam = 19,
    GenericParamConstraint = 20,
    MethodSpec = 21,
  };

  static constexpr TableFlag m[] = {
    TableFlag::MethodDef,
    TableFlag::Field,
    TableFlag::TypeRef,
    TableFlag::TypeDef,
    TableFlag::Param,
    TableFlag::InterfaceImpl,
    TableFlag::MemberRef,
    TableFlag::Module,
    TableFlag::Undefined,
    TableFlag::Property,
    TableFlag::Event,
    TableFlag::StandAloneSig,
    TableFlag::ModuleRef,
    TableFlag::TypeSpec,
    TableFlag::Assembly,
    TableFlag::AssemblyRef,
    TableFlag::File,
    TableFlag::ExportedType,
    TableFlag::ManifestResource,
    TableFlag::GenericParam,
    TableFlag::GenericParamConstraint,
    TableFlag::MethodSpec,
  };

  static constexpr size_t id = 2;  
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  HasCustomAttribute() = delete;
};

struct HasDeclSecurity {
  enum val_ {
    TypeDef = 0,
    MethodDef = 1,
    Assembly = 2,
  };

  static constexpr TableFlag m[] = {
    TableFlag::TypeDef,
    TableFlag::MethodDef,
    TableFlag::Assembly,
  };

  static constexpr size_t id = 3;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  HasDeclSecurity() = delete;
};

struct HasFieldMarshal {
  enum val_ {
    Field = 0,
    Param = 1,
  };

  static constexpr TableFlag m[] = {
    TableFlag::Field,
    TableFlag::Param,
  };

  static constexpr size_t id = 4;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  HasFieldMarshal() = delete;
};

struct HasSemantics {
  enum val_ {
    Event = 0,
    Property = 1,
  };

  static constexpr TableFlag m[] = {
    TableFlag::Event,
    TableFlag::Property,
  };

  static constexpr size_t id = 5;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  HasSemantics() = delete;
};

struct Implementation {
  enum val_ {
    File = 0,
    AssemblyRef = 1,
    ExportedType = 2,
  };

  static constexpr TableFlag m[] = {
    TableFlag::File,
    TableFlag::AssemblyRef,
    TableFlag::ExportedType,
  };

  static constexpr size_t id = 6;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  Implementation() = delete;
};

struct MemberForwarded {
  enum val_ {
    Field = 0,
    MethodDef = 1,
  };

  static constexpr TableFlag m[] = {
    TableFlag::Field,
    TableFlag::MethodDef,
  };

  static constexpr size_t id = 7;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  MemberForwarded() = delete;
};

struct MemberRefParent {
  enum val_ {
    TypeDef = 0,
    TypeRef = 1,
    ModuleRef = 2,
    MethodDef = 3,
    TypeSpec = 4,
  };

  static constexpr TableFlag m[] = {
    TableFlag::TypeDef,
    TableFlag::TypeRef,
    TableFlag::ModuleRef,
    TableFlag::MethodDef,
    TableFlag::TypeSpec,
  };

  static constexpr size_t id = 8;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  MemberRefParent() = delete;
};

struct MethodDefOrRef {
  enum val_ {
    MethodDef = 0,
    MemberRef = 1,
  };

  static constexpr TableFlag m[] = {
    TableFlag::MethodDef,
    TableFlag::MemberRef,
  };

  static constexpr size_t id = 9;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  MethodDefOrRef() = delete;
};

struct ResolutionScope {
  enum val_ {
    Module = 0,
    ModuleRef = 1,
    AssemblyRef = 2,
    TypeRef = 3,
  };

  static constexpr TableFlag m[] = {
    TableFlag::Module,
    TableFlag::ModuleRef,
    TableFlag::AssemblyRef,
    TableFlag::TypeRef,
  };

  static constexpr size_t id = 10;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  ResolutionScope() = delete;
};

struct TypeDefOrRef {
  enum val_ {
    TypeDef = 0,
    TypeRef = 1,
    TypeSpec = 2,
  };

  static constexpr TableFlag m[] = {
    TableFlag::TypeDef,
    TableFlag::TypeRef,
    TableFlag::TypeSpec,
  };

  static constexpr size_t id = 11;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  TypeDefOrRef() = delete;
};

struct TypeOrMethodDef {
  enum val_ {
    TypeDef = 0,
    MethodDef = 1,
  };

  static constexpr TableFlag m[] = {
    TableFlag::TypeDef,
    TableFlag::MethodDef,
  };

  static constexpr size_t id = 12;
  static constexpr size_t count = countof_(m);
  static constexpr size_t shift = bitcount(count - 1);

  TypeOrMethodDef() = delete;
};

#endif // DECLARATIONS_HPP_
