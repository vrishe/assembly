#pragma once

#ifndef DIAG_HPP_
#define DIAG_HPP_

#include "declarations.hpp"

static std::ostream& operator <<(std::ostream& dst, const HDR_MSDOS& src) {
  dst << "MS-DOS header" << std::endl;
  dst << " signature: ";
  dst.write(src.sig, countof_(src.sig)) << std::endl;
  dst << " offset:    " << src.e_lfanew << std::endl;

  return dst;
}

static std::ostream& operator <<(std::ostream& dst, const HDR_COFF& src) {
  dst << "COFF header" << std::endl;
  dst << " signature:     ";
  dst.write(src.sig, countof_(src.sig)) << std::endl;
  dst << " sections:      " << src.num_sections << std::endl;
  dst << " opt. hdr size: " << src.sz_hdropt << std::endl;
  dst << " flags:         0x" << std::setw(4) << std::hex << src.flags << std::endl;

  using namespace std::chrono;
  auto time = system_clock::to_time_t(
    time_point<system_clock>(seconds(src.timestamp)));

  dst << " timestamp:     " << ctime(&time) << std::endl;

  return dst;
}

#define TABLE_FLAG_CASE_(flag) \
case TableFlag::flag: dst << #flag; break
static std::ostream& operator <<(std::ostream& dst, TableFlag src) {
  switch (src) {
TABLE_FLAG_CASE_(Assembly);
TABLE_FLAG_CASE_(AssemblyOS);
TABLE_FLAG_CASE_(AssemblyProcessor);
TABLE_FLAG_CASE_(AssemblyRef);
TABLE_FLAG_CASE_(AssemblyRefOS);
TABLE_FLAG_CASE_(AssemblyRefProcessor);
TABLE_FLAG_CASE_(ClassLayout);
TABLE_FLAG_CASE_(Constant);
TABLE_FLAG_CASE_(CustomAttribute);
TABLE_FLAG_CASE_(DeclSecurity);
TABLE_FLAG_CASE_(EventMap);
TABLE_FLAG_CASE_(Event);
TABLE_FLAG_CASE_(ExportedType);
TABLE_FLAG_CASE_(Field);
TABLE_FLAG_CASE_(FieldLayout);
TABLE_FLAG_CASE_(FieldMarshal);
TABLE_FLAG_CASE_(FieldRVA);
TABLE_FLAG_CASE_(File);
TABLE_FLAG_CASE_(GenericParam);
TABLE_FLAG_CASE_(GenericParamConstraint);
TABLE_FLAG_CASE_(ImplMap);
TABLE_FLAG_CASE_(InterfaceImpl);
TABLE_FLAG_CASE_(ManifestResource);
TABLE_FLAG_CASE_(MemberRef);
TABLE_FLAG_CASE_(MethodDef);
TABLE_FLAG_CASE_(MethodImpl);
TABLE_FLAG_CASE_(MethodSemantics);
TABLE_FLAG_CASE_(MethodSpec);
TABLE_FLAG_CASE_(Module);
TABLE_FLAG_CASE_(ModuleRef);
TABLE_FLAG_CASE_(NestedClass);
TABLE_FLAG_CASE_(Param);
TABLE_FLAG_CASE_(Property);
TABLE_FLAG_CASE_(PropertyMap);
TABLE_FLAG_CASE_(StandAloneSig);
TABLE_FLAG_CASE_(TypeDef);
TABLE_FLAG_CASE_(TypeRef);
TABLE_FLAG_CASE_(TypeSpec);
    default:
      dst << "<UNKNOWN>";  
  }
  return dst;
}

static std::ostream& operator <<(std::ostream& dst, const guid& guid) {
  std::stringstream receiver;
  receiver << std::hex;

  auto b = &guid[0];

  receiver << std::setfill('0') << std::setw(2) << *reinterpret_cast<const dword*>(b);
  b += sizeof(dword);
  receiver << "-";

  receiver << std::setfill('0') << std::setw(2) << *reinterpret_cast<const word*>(b);
  b += sizeof(word);
  receiver << "-";

  receiver << std::setfill('0') << std::setw(2) << *reinterpret_cast<const word*>(b);
  b += sizeof(word);
  receiver << "-";

  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << "-";

  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << std::setfill('0') << std::setw(2) << int(*b++);
  receiver << std::setfill('0') << std::setw(2) << int(*b  );

  return dst << "{" << receiver.str() << "}";
}

#ifdef DIAG
#define DIAGNOSTICS(code) \
 do { code } while(false);
#else // !DIAG
#define DIAGNOSTICS(code) // Nothing
#endif // DIAG

#endif // DIAG_HPP_