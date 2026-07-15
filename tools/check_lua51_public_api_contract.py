#!/usr/bin/env python3
"""Validate the frozen Lua 5.1.5 public-function conformance contract."""

from __future__ import annotations

import json
import re
import sys
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "tests" / "compatibility" / "lua51-public-api-contract.json"
WINDOWS_EXPORTS = ROOT / "tests" / "compatibility" / "lua_public_api_exports.def"
UNIX_EXPORTS = ROOT / "tests" / "compatibility" / "lua_public_api_exports.map"
PUBLIC_C_PROBE = ROOT / "tests" / "compatibility" / "public_api_c_compile.c"
PUBLIC_CPP_PROBE = ROOT / "tests" / "compatibility" / "public_api_cpp_consumer.cpp"
C_API_DIFFERENTIAL_PROBE = ROOT / "tests" / "compatibility" / "lua51_c_api_differential_probe.c"
C_API_DIFFERENTIAL_RUNNER = ROOT / "tools" / "run_lua51_c_api_differential.ps1"
BUILD_DEFINITION = ROOT / "CMakeLists.txt"
ALLOWED_STATUSES = {"PASS", "XFAIL", "UNSUPPORTED"}
ISSUE_URL = re.compile(r"https://github\.com/YanqingXu/lua/issues/[1-9][0-9]*$")
PUBLIC_HEADER_INFRASTRUCTURE_MACROS = {
    "LAUXLIB_H",
    "LUALIB_H",
    "LUA_CXX_MAY_THROW",
    "LUA_CXX_NOEXCEPT",
    "LUA_H",
}
OFFICIAL_FUNCTIONS_BY_HEADER = {
    "lua.h": frozenset(
        {
            "lua_newstate",
            "lua_close",
            "lua_newthread",
            "lua_atpanic",
            "lua_gettop",
            "lua_settop",
            "lua_pushvalue",
            "lua_remove",
            "lua_insert",
            "lua_replace",
            "lua_checkstack",
            "lua_xmove",
            "lua_isnumber",
            "lua_isstring",
            "lua_iscfunction",
            "lua_isuserdata",
            "lua_type",
            "lua_typename",
            "lua_equal",
            "lua_rawequal",
            "lua_lessthan",
            "lua_tonumber",
            "lua_tointeger",
            "lua_toboolean",
            "lua_tolstring",
            "lua_objlen",
            "lua_tocfunction",
            "lua_touserdata",
            "lua_tothread",
            "lua_topointer",
            "lua_pushnil",
            "lua_pushnumber",
            "lua_pushinteger",
            "lua_pushlstring",
            "lua_pushstring",
            "lua_pushvfstring",
            "lua_pushfstring",
            "lua_pushcclosure",
            "lua_pushboolean",
            "lua_pushlightuserdata",
            "lua_pushthread",
            "lua_gettable",
            "lua_getfield",
            "lua_rawget",
            "lua_rawgeti",
            "lua_createtable",
            "lua_newuserdata",
            "lua_getmetatable",
            "lua_getfenv",
            "lua_settable",
            "lua_setfield",
            "lua_rawset",
            "lua_rawseti",
            "lua_setmetatable",
            "lua_setfenv",
            "lua_call",
            "lua_pcall",
            "lua_cpcall",
            "lua_load",
            "lua_dump",
            "lua_yield",
            "lua_resume",
            "lua_status",
            "lua_gc",
            "lua_error",
            "lua_next",
            "lua_concat",
            "lua_getallocf",
            "lua_setallocf",
            "lua_setlevel",
            "lua_getstack",
            "lua_getinfo",
            "lua_getlocal",
            "lua_setlocal",
            "lua_getupvalue",
            "lua_setupvalue",
            "lua_sethook",
            "lua_gethook",
            "lua_gethookmask",
            "lua_gethookcount",
        }
    ),
    "lauxlib.h": frozenset(
        {
            "luaL_openlib",
            "luaL_register",
            "luaL_getmetafield",
            "luaL_callmeta",
            "luaL_typerror",
            "luaL_argerror",
            "luaL_checklstring",
            "luaL_optlstring",
            "luaL_checknumber",
            "luaL_optnumber",
            "luaL_checkinteger",
            "luaL_optinteger",
            "luaL_checkstack",
            "luaL_checktype",
            "luaL_checkany",
            "luaL_newmetatable",
            "luaL_checkudata",
            "luaL_where",
            "luaL_error",
            "luaL_checkoption",
            "luaL_ref",
            "luaL_unref",
            "luaL_loadfile",
            "luaL_loadbuffer",
            "luaL_loadstring",
            "luaL_newstate",
            "luaL_gsub",
            "luaL_findtable",
            "luaL_buffinit",
            "luaL_prepbuffer",
            "luaL_addlstring",
            "luaL_addstring",
            "luaL_addvalue",
            "luaL_pushresult",
        }
    ),
    "lualib.h": frozenset(
        {
            "luaopen_base",
            "luaopen_table",
            "luaopen_io",
            "luaopen_os",
            "luaopen_string",
            "luaopen_math",
            "luaopen_debug",
            "luaopen_package",
            "luaL_openlibs",
        }
    ),
}
PROJECT_PUBLIC_HEADERS = tuple(ROOT / "src" / name for name in ("lua.h", "lauxlib.h", "lualib.h"))
PROJECT_IMPLEMENTATION_SOURCES = tuple((ROOT / "src").rglob("*.cpp"))


def fail(message: str) -> None:
    raise RuntimeError(message)


def contains_function(text: str, symbol: str) -> bool:
    return re.search(rf"\b{re.escape(symbol)}\s*\(", text) is not None


def contains_function_definition(text: str, symbol: str) -> bool:
    return re.search(
        rf"(?ms)^[ \t]*[A-Za-z_][^;{{}}\n]*\b{re.escape(symbol)}\s*\([^;{{}}]*\)"
        rf"\s*(?:LUA_CXX_[A-Z_]+\s*)?\{{",
        text,
    ) is not None


def format_symbols(symbols: set[str] | frozenset[str]) -> str:
    return ", ".join(sorted(symbols)) or "<none>"


def read_windows_exports() -> set[str]:
    exports: set[str] = set()
    saw_exports = False
    for raw_line in WINDOWS_EXPORTS.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith(";"):
            continue
        if not saw_exports:
            if line != "EXPORTS":
                fail("Windows public API export definition must start with EXPORTS")
            saw_exports = True
            continue
        symbol = line.split()[0]
        if symbol in exports:
            fail(f"duplicate Windows export: {symbol}")
        exports.add(symbol)
    if not saw_exports:
        fail("Windows public API export definition has no EXPORTS section")
    return exports


def read_unix_exports() -> set[str]:
    exports: set[str] = set()
    for match in re.finditer(r"(?m)^\s*(lua[A-Za-z0-9_]*)\s*;\s*$", UNIX_EXPORTS.read_text(encoding="utf-8")):
        symbol = match.group(1)
        if symbol in exports:
            fail(f"duplicate Unix export: {symbol}")
        exports.add(symbol)
    if not exports:
        fail("Unix public API version script has no Lua exports")
    return exports


def project_public_functions(header_texts: dict[Path, str]) -> set[str]:
    declarations: set[str] = set()
    pattern = re.compile(
        r"(?m)^[ \t]*(?!typedef\b)(?!#)[A-Za-z_][A-Za-z0-9_ \t*]*\b(lua[A-Za-z0-9_]*)"
        r"\s*\([^;{}\n]*\)(?:\s+LUA_CXX_(?:MAY_THROW|NOEXCEPT))?\s*;"
    )
    for text in header_texts.values():
        declarations.update(pattern.findall(text))
    if not declarations:
        fail("could not enumerate project public function declarations")
    return declarations


def project_public_macros(header_texts: dict[Path, str]) -> set[str]:
    macros: set[str] = set()
    for text in header_texts.values():
        macros.update(re.findall(r"(?m)^#define\s+((?:LUA|lua)[A-Za-z0-9_]*)", text))
    return macros - PUBLIC_HEADER_INFRASTRUCTURE_MACROS


def project_public_constants(header_texts: dict[Path, str]) -> set[str]:
    constants: set[str] = set()
    for text in header_texts.values():
        for enum_body in re.findall(r"\benum(?:\s+[A-Za-z_][A-Za-z0-9_]*)?\s*{([^}]*)}", text, re.DOTALL):
            constants.update(re.findall(r"\b(LUA_[A-Z0-9_]+)\b", enum_body))
    return constants


def project_public_types(header_texts: dict[Path, str]) -> set[str]:
    public_types: set[str] = set()
    for text in header_texts.values():
        public_types.update(re.findall(r"typedef\s+[^;{}()]*\b(lua[A-Za-z0-9_]*)\s*;", text))
        public_types.update(re.findall(r"typedef\s+[^;{}]*\(\s*\*\s*(lua[A-Za-z0-9_]*)\s*\)", text))
        public_types.update(re.findall(r"}\s*(lua[A-Za-z0-9_]*)\s*;", text))
    if not public_types:
        fail("could not enumerate project public typedefs")
    return public_types


def marker_symbols(text: str, marker: str) -> set[str]:
    return set(re.findall(rf"\b{re.escape(marker)}\s*\(\s*(lua[A-Za-z0-9_]*|LUA_[A-Z0-9_]*)\s*[,)]", text))


def require_exact_surface(label: str, actual: set[str], expected: set[str]) -> None:
    if actual != expected:
        missing = expected - actual
        extra = actual - expected
        fail(f"{label} drifted; missing [{format_symbols(missing)}], extra [{format_symbols(extra)}]")


def main() -> int:
    data = json.loads(CONTRACT.read_text(encoding="utf-8"))
    if data.get("schemaVersion") != 1:
        fail("unsupported contract schemaVersion")

    entries = data.get("entries")
    if not isinstance(entries, list):
        fail("entries must be a list")

    frozen_counts = {header: len(symbols) for header, symbols in OFFICIAL_FUNCTIONS_BY_HEADER.items()}
    expected_counts = data.get("expectedFunctionCounts")
    if expected_counts != frozen_counts:
        fail(f"expectedFunctionCounts drifted from the frozen Lua 5.1.5 set: {expected_counts!r}")
    actual_counts = Counter(entry.get("header") for entry in entries)
    if actual_counts != Counter(expected_counts):
        fail(f"official header counts drifted: expected {expected_counts}, got {dict(actual_counts)}")

    symbols = [entry.get("symbol") for entry in entries]
    if len(symbols) != len(set(symbols)):
        fail("contract contains duplicate symbols")

    for header, official_symbols in OFFICIAL_FUNCTIONS_BY_HEADER.items():
        contract_symbols = {entry.get("symbol") for entry in entries if entry.get("header") == header}
        missing = official_symbols - contract_symbols
        unexpected = contract_symbols - official_symbols
        if missing or unexpected:
            fail(
                f"{header}: contract differs from the frozen Lua 5.1.5 function set; "
                f"missing [{format_symbols(missing)}], unexpected [{format_symbols(unexpected)}]"
            )

    public_header_texts = {path: path.read_text(encoding="utf-8") for path in PROJECT_PUBLIC_HEADERS}
    implementation_texts = {path: path.read_text(encoding="utf-8") for path in PROJECT_IMPLEMENTATION_SOURCES}
    declared_functions = project_public_functions(public_header_texts)
    declared_macros = project_public_macros(public_header_texts)
    declared_constants = project_public_constants(public_header_texts)
    declared_types = project_public_types(public_header_texts)
    c_probe_text = PUBLIC_C_PROBE.read_text(encoding="utf-8")
    cpp_probe_text = PUBLIC_CPP_PROBE.read_text(encoding="utf-8")

    require_exact_surface(
        "C link-probe function surface", marker_symbols(c_probe_text, "REQUIRE_SYMBOL"), declared_functions
    )
    require_exact_surface(
        "C++ exact-signature surface", marker_symbols(cpp_probe_text, "REQUIRE_SIGNATURE"), declared_functions
    )
    require_exact_surface(
        "C++ public-macro surface", marker_symbols(cpp_probe_text, "REQUIRE_PUBLIC_MACRO"), declared_macros
    )
    require_exact_surface(
        "C++ public-constant surface",
        marker_symbols(cpp_probe_text, "REQUIRE_PUBLIC_CONSTANT"),
        declared_constants,
    )
    require_exact_surface(
        "C++ public-type surface", marker_symbols(cpp_probe_text, "REQUIRE_PUBLIC_TYPE"), declared_types
    )

    missing_definitions = {
        symbol
        for symbol in declared_functions
        if not any(contains_function_definition(text, symbol) for text in implementation_texts.values())
    }
    if missing_definitions:
        fail(f"public declarations without definitions: [{format_symbols(missing_definitions)}]")

    status_counts: Counter[str] = Counter()
    c_probe_texts: dict[Path, str] = {}
    link_texts: dict[Path, str] = {}
    for entry in entries:
        symbol = entry.get("symbol")
        header = entry.get("header")
        status = entry.get("status")
        if not isinstance(symbol, str) or not symbol.startswith("lua"):
            fail(f"invalid symbol entry: {entry!r}")
        if status not in ALLOWED_STATUSES:
            fail(f"{symbol}: invalid status {status!r}")
        status_counts[status] += 1

        if status in {"PASS", "XFAIL"}:
            declaration_path = ROOT / entry["declaredIn"]
            definition_path = ROOT / entry["definedIn"]
            c_probe_path = ROOT / entry["cCompileEvidence"]
            link_path = ROOT / entry["linkEvidence"]
            runtime_path = ROOT / entry["runtimeEvidence"]
            for evidence_path in (declaration_path, definition_path, c_probe_path, link_path, runtime_path):
                if not evidence_path.is_file():
                    fail(f"{symbol}: missing evidence file {evidence_path.relative_to(ROOT)}")

            c_probe_text = c_probe_texts.setdefault(c_probe_path, c_probe_path.read_text(encoding="utf-8"))
            link_text = link_texts.setdefault(link_path, link_path.read_text(encoding="utf-8"))

            if not contains_function(declaration_path.read_text(encoding="utf-8"), symbol):
                fail(f"{symbol}: PASS symbol is not declared in {entry['declaredIn']}")
            if not contains_function_definition(definition_path.read_text(encoding="utf-8"), symbol):
                fail(f"{symbol}: PASS symbol is not defined in {entry['definedIn']}")
            if not re.search(rf"\bREQUIRE_SYMBOL\s*\(\s*{re.escape(symbol)}\s*\)", c_probe_text):
                fail(f"{symbol}: C compile probe does not reference the symbol")
            if "lua_public_c_header_probe()" not in link_text:
                fail(f"{symbol}: optimized consumer does not invoke the C link probe")
            if not contains_function(runtime_path.read_text(encoding="utf-8"), symbol):
                fail(f"{symbol}: direct runtime evidence does not call the public entry point")
            if entry.get("semanticStatus") != status:
                fail(f"{symbol}: {status} entry must have semanticStatus={status}")
            if status == "XFAIL":
                mismatch_path = ROOT / entry["mismatchEvidence"]
                if not mismatch_path.is_file():
                    fail(f"{symbol}: XFAIL mismatch evidence is missing")
                if not entry.get("reason") or not ISSUE_URL.fullmatch(entry.get("tracking", "")):
                    fail(f"{symbol}: XFAIL needs an explicit reason and GitHub issue URL")
        else:
            if not entry.get("reason") or not ISSUE_URL.fullmatch(entry.get("tracking", "")):
                fail(f"{symbol}: UNSUPPORTED needs an explicit reason and GitHub issue URL")
            if status == "UNSUPPORTED":
                declaring_headers = [
                    path.relative_to(ROOT) for path, text in public_header_texts.items() if contains_function(text, symbol)
                ]
                defining_sources = [
                    path.relative_to(ROOT)
                    for path, text in implementation_texts.items()
                    if contains_function_definition(text, symbol)
                ]
                if declaring_headers or defining_sources:
                    locations = declaring_headers + defining_sources
                    fail(
                        f"{symbol}: UNSUPPORTED entry is present in project API code: "
                        + ", ".join(str(path) for path in locations)
                    )

    if status_counts["PASS"] != 75 or status_counts["UNSUPPORTED"] != 48:
        fail(f"unexpected status partition: {dict(status_counts)}")

    for path, text in c_probe_texts.items():
        if not re.search(r"volatile\s+lua_CFunction\s+lua_public_link_sink", text):
            fail(f"{path.relative_to(ROOT)}: C link probe is missing its volatile function-pointer sink")
        if not re.search(r"lua_public_link_sink\s*=\s*\(lua_CFunction\)\s*\(name\)", text):
            fail(f"{path.relative_to(ROOT)}: REQUIRE_SYMBOL does not create an evaluated link reference")

    pass_symbols = {entry["symbol"] for entry in entries if entry["status"] == "PASS"}
    if not pass_symbols <= declared_functions:
        fail(f"official PASS functions missing from public headers: [{format_symbols(pass_symbols - declared_functions)}]")

    windows_exports = read_windows_exports()
    require_exact_surface("Windows public exports", windows_exports, declared_functions)
    require_exact_surface("Unix public exports", read_unix_exports(), declared_functions)

    build_text = BUILD_DEFINITION.read_text(encoding="utf-8")
    for required_build_evidence in (
        "add_library(lua_public_api_shared SHARED",
        "lua_public_api_exports.def",
        "lua_public_api_exports.map",
        "add_executable(lua_public_api_shared_consumer",
        "target_link_libraries(lua_public_api_shared_consumer PRIVATE lua_public_api_shared)",
        "add_executable(lua51_c_api_differential_probe",
        "target_link_libraries(lua51_c_api_differential_probe PRIVATE lua_core)",
    ):
        if required_build_evidence not in build_text:
            fail(f"shared-library ABI evidence is missing from CMakeLists.txt: {required_build_evidence}")

    differential_text = C_API_DIFFERENTIAL_PROBE.read_text(encoding="utf-8")
    for symbol in {
        "lua_concat",
        "lua_equal",
        "lua_getfield",
        "lua_lessthan",
        "lua_next",
        "lua_gc",
        "lua_pushthread",
        "lua_rawequal",
        "lua_rawget",
        "lua_rawset",
        "lua_setfield",
        "lua_tocfunction",
        "lua_tointeger",
        "lua_topointer",
        "lua_tothread",
    }:
        if not contains_function(differential_text, symbol):
            fail(f"{symbol}: C API differential probe does not call the public entry point")
    if not C_API_DIFFERENTIAL_RUNNER.is_file():
        fail("Lua 5.1 C API differential runner is missing")

    print(
        "Lua 5.1.5 public API contract OK: "
        f"{len(entries)} official functions, {status_counts['PASS']} PASS, "
        f"{status_counts['XFAIL']} XFAIL, {status_counts['UNSUPPORTED']} UNSUPPORTED; "
        f"project surface {len(declared_functions)} functions, {len(declared_macros)} macros, "
        f"{len(declared_constants)} enum constants, {len(declared_types)} typedefs"
    )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (KeyError, OSError, ValueError, RuntimeError) as error:
        print(f"public API contract check failed: {error}", file=sys.stderr)
        sys.exit(1)
