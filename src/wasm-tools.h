#include <map>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string_view>
#include <vector>

namespace WasmTools {

using std::operator""s;

inline const uint8_t wasm_sec_custom = 0;
inline const uint8_t wasm_sec_type = 1;
inline const uint8_t wasm_sec_import = 2;
inline const uint8_t wasm_sec_function = 3;
inline const uint8_t wasm_sec_table = 4;
inline const uint8_t wasm_sec_memory = 5;
inline const uint8_t wasm_sec_global = 6;
inline const uint8_t wasm_sec_export = 7;
inline const uint8_t wasm_sec_start = 8;
inline const uint8_t wasm_sec_elem = 9;
inline const uint8_t wasm_sec_code = 10;
inline const uint8_t wasm_sec_data = 11;
inline const uint8_t num_sections = 12;

inline const uint8_t external_function = 0;
inline const uint8_t external_table = 1;
inline const uint8_t external_memory = 2;
inline const uint8_t external_global = 3;

inline const uint8_t type_i32 = 0x7f;
inline const uint8_t type_i64 = 0x7e;
inline const uint8_t type_f32 = 0x7d;
inline const uint8_t type_f64 = 0x7c;
inline const uint8_t type_anyfunc = 0x70;
inline const uint8_t type_func = 0x60;
inline const uint8_t type_block = 0x40;

inline const uint8_t r_webassembly_function_index_leb = 0;
inline const uint8_t r_webassembly_table_index_sleb = 1;
inline const uint8_t r_webassembly_table_index_i32 = 2;
inline const uint8_t r_webassembly_memory_addr_leb = 3;
inline const uint8_t r_webassembly_memory_addr_sleb = 4;
inline const uint8_t r_webassembly_memory_addr_i32 = 5;
inline const uint8_t r_webassembly_type_index_leb = 6;
inline const uint8_t r_webassembly_global_index_leb = 7;

inline const uint8_t wasm_symbol_info = 0x2;
inline const uint8_t wasm_data_size = 0x3;
inline const uint8_t wasm_data_alignment = 0x4;
inline const uint8_t wasm_segment_info = 0x5;

auto type_str(uint8_t type) {
    switch (type) {
    case type_i32:
        return "i32";
    case type_i64:
        return "i64";
    case type_f32:
        return "f32";
    case type_f64:
        return "f64";
    case type_anyfunc:
        return "anyfunc";
    case type_func:
        return "func";
    case type_block:
        return "block";
    default:
        return "bad_type";
    }
}

inline const uint8_t name_module = 0;
inline const uint8_t name_function = 1;
inline const uint8_t name_local = 2;

struct File {
    FILE* file;
    File(const char* name, const char* mode) {
        file = fopen(name, mode);
        if (!file)
            throw std::runtime_error{("can not open "s + name).c_str()};
    }
    ~File() { fclose(file); }

    size_t size() {
        fseek(file, 0, SEEK_END);
        auto result = ftell(file);
        fseek(file, 0, SEEK_SET);
        return result;
    }

    auto read() {
        auto s = size();
        auto buffer = std::vector<uint8_t>(s);
        if (fread(&buffer[0], s, 1, file) != 1)
            throw std::runtime_error{"Read failed"};
        return buffer;
    }

    void write(const std::vector<uint8_t>& buffer) {
        if (fwrite(&buffer[0], buffer.size(), 1, file) != 1)
            throw std::runtime_error{"Write failed"};
    }
};

inline auto readLeb(const std::vector<uint8_t>& binary, size_t& pos) {
    auto result = uint32_t{0};
    auto shift = 0;
    while (true) {
        auto b = binary[pos++];
        result |= (uint32_t{b} & 0x7f) << shift;
        shift += 7;
        if (!(b & 0x80))
            return result;
    }
}

inline void pushLeb5(std::vector<uint8_t>& binary, uint32_t value) {
    binary.push_back(((value >> 0) & 0x7f) | 0x80);
    binary.push_back(((value >> 7) & 0x7f) | 0x80);
    binary.push_back(((value >> 14) & 0x7f) | 0x80);
    binary.push_back(((value >> 21) & 0x7f) | 0x80);
    binary.push_back(((value >> 28) & 0x1f) | 0);
}

inline std::string_view readStr(const std::vector<uint8_t>& binary,
                                size_t& pos) {
    auto len = readLeb(binary, pos);
    auto str = std::string_view{(char*)&binary[0] + pos, len};
    pos += len;
    return str;
}

template <typename T> void check(bool cond, const T& msg) {
    if (!cond)
        throw std::runtime_error(msg);
}

inline auto getInitExpr32(const std::vector<uint8_t>& binary, size_t& pos) {
    check(binary[pos++] == 0x41, "init_expr is not i32.const");
    auto offset = readLeb(binary, pos);
    check(binary[pos++] == 0x0b, "init_expr missing end");
    return offset;
}

struct Section {
    bool valid{};
    size_t begin{};
    size_t end{};
};

struct CustomSection {
    bool valid{};
    std::string_view name{};
    size_t begin{};
    size_t end{};
};

struct Import {
    std::string_view name{};
    uint8_t kind{};
    uint32_t index{};
};

struct FunctionType {
    std::vector<uint8_t> argTypes{};
    std::vector<uint8_t> returnTypes{};
};

struct Function {
    uint32_t type{};
};

struct ResizableLimits {
    bool valid{};
    bool max_present{};
    uint32_t initial{};
    uint32_t maximum{};
};

struct Global {
    bool isStack{};
    uint32_t initU32{};
};

struct Export {
    std::string_view name{};
    uint8_t kind{};
    uint32_t index{};
};

struct Element {
    bool valid{};
    uint32_t fIndex{};
};

struct DataSegment {
    uint32_t offset;
    uint32_t size;
    uint32_t dataBegin;
};

struct Reloc {
    uint32_t section_id{};
    uint32_t type{};
    uint32_t offset{};
    uint32_t index{};
    uint32_t addend{};
};

struct Symbol {
    uint32_t flags{};
    std::vector<uint32_t> imports{};
    std::vector<uint32_t> exports{};
    bool inLinking{};
};

struct Module {
    std::vector<uint8_t> binary;
    Section sections[num_sections]{};
    std::vector<Import> imports{};
    std::vector<ResizableLimits> tables{};
    std::vector<ResizableLimits> memories{};
    uint32_t numImportedGlobals{};
    std::vector<Global> globals{};
    std::vector<FunctionType> functionTypes{};
    uint32_t numImportedFunctions{};
    std::vector<Function> functions{};
    std::vector<Export> exports{};
    std::vector<Element> elements{};
    std::vector<DataSegment> dataSegments{};
    std::vector<Reloc> relocs{};
    std::map<std::string_view, Symbol> symbols{};
    uint32_t dataSize{};
};

inline ResizableLimits read_resizable_limits(const std::vector<uint8_t>& binary,
                                             size_t& pos) {
    auto max_present = !!(binary[pos++] & 1);
    auto initial = readLeb(binary, pos);
    uint32_t maximum{};
    if (max_present)
        maximum = readLeb(binary, pos);
    return {true, max_present, initial, maximum};
}

inline void read_sec_type(Module& module, size_t& pos, size_t sEnd) {
    printf("type\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        FunctionType functionType;
        check(module.binary[pos++] == type_func, "invalid form in type");
        printf("    [%03d] type (", i);
        auto param_count = readLeb(module.binary, pos);
        for (uint32_t j = 0; j < param_count; ++j) {
            functionType.argTypes.push_back(module.binary[pos++]);
            printf("%s ", type_str(functionType.argTypes.back()));
        }
        printf(") ==> (");
        auto return_count = readLeb(module.binary, pos);
        for (uint32_t j = 0; j < return_count; ++j) {
            functionType.returnTypes.push_back(module.binary[pos++]);
            printf("%s ", type_str(functionType.returnTypes.back()));
        }
        check(functionType.returnTypes.size() <= 1, "multiple return types");
        printf(")\n");
        module.functionTypes.push_back(std::move(functionType));
    }
}

inline void read_sec_import(Module& module, size_t& pos, size_t sEnd) {
    printf("import\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        auto moduleName = readStr(module.binary, pos);
        auto fieldName = readStr(module.binary, pos);
        auto kind = module.binary[pos++];
        switch (kind) {
        case external_function: {
            auto type = readLeb(module.binary, pos);
            check(type < module.functionTypes.size(),
                  "function type doesn't exist");
            printf("    [%03zu] func   %s.%s type %d\n",
                   module.functions.size(), std::string{moduleName}.c_str(),
                   std::string{fieldName}.c_str(), type);
            ++module.numImportedFunctions;
            module.symbols[fieldName].imports.push_back(
                uint32_t(module.imports.size()));
            module.imports.push_back(
                Import{fieldName, kind, (uint32_t)module.functions.size()});
            module.functions.push_back({type});
            break;
        }
        case external_table: {
            check(module.binary[pos++] == type_anyfunc,
                  "import table is not anyfunc");
            check(!module.tables.size(), "multiple tables");
            auto limits = read_resizable_limits(module.binary, pos);
            printf("    [000] table  %s.%s max_present:%u initial:%u max:%u\n",
                   std::string{moduleName}.c_str(),
                   std::string{fieldName}.c_str(), limits.max_present,
                   limits.initial, limits.maximum);
            module.symbols[fieldName].imports.push_back(
                uint32_t(module.imports.size()));
            module.imports.push_back(
                Import{fieldName, kind, (uint32_t)module.tables.size()});
            module.tables.push_back(limits);
            break;
        }
        case external_memory: {
            check(!module.memories.size(), "multiple memories");
            auto limits = read_resizable_limits(module.binary, pos);
            printf("    [000] memory %s.%s max_present:%u initial:%u max:%u\n",
                   std::string{moduleName}.c_str(),
                   std::string{fieldName}.c_str(), limits.max_present,
                   limits.initial, limits.maximum);
            module.symbols[fieldName].imports.push_back(
                uint32_t(module.imports.size()));
            module.imports.push_back(
                Import{fieldName, kind, (uint32_t)module.memories.size()});
            module.memories.push_back(limits);
            break;
        }
        case external_global: {
            check(module.binary[pos++] == type_i32,
                  "imported global is not i32");
            auto mutability = module.binary[pos++];
            printf("    [%03zu] global %s.%s %s\n", module.globals.size(),
                   std::string{moduleName}.c_str(),
                   std::string{fieldName}.c_str(), mutability ? "mut" : "");
            ++module.numImportedGlobals;
            module.symbols[fieldName].imports.push_back(
                uint32_t(module.imports.size()));
            module.imports.push_back(
                Import{fieldName, kind, (uint32_t)module.globals.size()});
            module.globals.push_back({fieldName == "__stack_pointer"});
            break;
        }
        default:
            check(false, "unknown import kind");
        }
    }
    check(pos == sEnd, "import section malformed");
} // read_sec_import

inline void read_sec_function(Module& module, size_t& pos, size_t sEnd) {
    printf("function\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        auto type = readLeb(module.binary, pos);
        check(type < module.functionTypes.size(),
              "function type doesn't exist");
        printf("    [%03zu] func type=%d\n", module.functions.size(), type);
        module.functions.push_back({type});
    }
    check(pos == sEnd, "function section malformed");
} // read_sec_function

inline void read_sec_table(Module& module, size_t& pos, size_t sEnd) {
    check(module.binary[pos++] == type_anyfunc, "import table is not anyfunc");
    check(!module.tables.size(), "multiple tables");
    auto limits = read_resizable_limits(module.binary, pos);
    printf("    [000] table  max_present:%u initial:%u max:%u\n",
           limits.max_present, limits.initial, limits.maximum);
    module.tables.push_back(limits);
    check(pos == sEnd, "table section malformed");
}

inline void read_sec_memory(Module& module, size_t& pos, size_t sEnd) {
    check(!module.memories.size(), "multiple memories");
    auto limits = read_resizable_limits(module.binary, pos);
    printf("    [000] memory max_present:%u initial:%u max:%u\n",
           limits.max_present, limits.initial, limits.maximum);
    module.memories.push_back(limits);
    check(pos == sEnd, "memory section malformed");
}

inline void read_sec_global(Module& module, size_t& pos, size_t sEnd) {
    printf("global\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        check(module.binary[pos++] == type_i32, "global is not i32");
        auto mutability = module.binary[pos++];
        auto initU32 = getInitExpr32(module.binary, pos);
        printf("    [%03zu] global %s = %u\n", module.globals.size(),
               mutability ? "mut" : "", initU32);
        module.globals.push_back({false, initU32});
    }
    check(pos == sEnd, "global section malformed");
}

inline void read_sec_export(Module& module, size_t& pos, size_t sEnd) {
    printf("export\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        auto name = readStr(module.binary, pos);
        auto kind = module.binary[pos++];
        auto index = readLeb(module.binary, pos);
        if (kind == external_function) {
            check(index >= module.numImportedFunctions &&
                      index < module.functions.size(),
                  "export has invalid function index");
            module.symbols[name].exports.push_back(
                uint32_t(module.exports.size()));
            module.exports.push_back({name, kind, index});
            printf("    [%03u] func   %s\n", index, std::string{name}.c_str());
        } else if (kind == external_global) {
            check(index >= module.numImportedGlobals &&
                      index < module.globals.size(),
                  "export has invalid global index");
            module.symbols[name].exports.push_back(
                uint32_t(module.exports.size()));
            module.exports.push_back({name, kind, index});
            printf("    [%03u] global %s\n", index, std::string{name}.c_str());
        } else {
            printf("    [---] skipped\n");
        }
    }
    check(pos == sEnd, "export section malformed");
}

inline void read_sec_start(Module& module, size_t& pos, size_t sEnd) {
    check(false, "start section unsupported");
}

inline void read_sec_elem(Module& module, size_t& pos, size_t sEnd) {
    printf("elem\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        check(readLeb(module.binary, pos) == 0, "elem table index not 0");
        auto offset = getInitExpr32(module.binary, pos);
        auto num = readLeb(module.binary, pos);
        for (uint32_t j = 0; j < num; ++j) {
            auto eIndex = offset + j;
            auto fIndex = readLeb(module.binary, pos);
            check(fIndex < module.functions.size(),
                  "elem has invalid function index");
            printf("    [%03u] = [%03u] func\n", eIndex, fIndex);
            if (eIndex >= module.elements.size())
                module.elements.resize(eIndex + 1);
            module.elements[eIndex] = {Element{true, fIndex}};
        }
    }
    for (size_t i = 0; i < module.elements.size(); ++i)
        if (!module.elements[i].valid)
            check(false, "hole in table at index " + std::to_string(i));
    check(pos == sEnd, "elem section malformed");
}

inline void read_sec_code(Module& module, size_t& pos, size_t sEnd) {
    printf("code\n");
}

inline void read_sec_data(Module& module, size_t& pos, size_t sEnd) {
    printf("data\n");
    auto count = readLeb(module.binary, pos);
    for (uint32_t i = 0; i < count; ++i) {
        check(readLeb(module.binary, pos) == 0, "data memory index not 0");
        auto offset = getInitExpr32(module.binary, pos);
        auto size = readLeb(module.binary, pos);
        printf("    offset:%d size=%d\n", offset, size);
        module.dataSegments.push_back(DataSegment{offset, size, uint32_t(pos)});
        pos += size;
    }
    auto lastUsed = uint32_t{0};
    for (auto& seg : module.dataSegments) {
        check(seg.offset == lastUsed, "segments not contiguous");
        lastUsed += seg.size;
    }
    check(pos == sEnd, "data section malformed");
}

inline void read_sec_name(Module& module, size_t& pos, size_t sEnd) {
    printf("name\n");
    while (pos < sEnd) {
        auto type = module.binary[pos++];
        auto subLen = readLeb(module.binary, pos);
        auto subEnd = pos + subLen;
        printf("    type %d\n", type);
        if (type == name_function) {
            auto count = readLeb(module.binary, pos);
            for (uint32_t i = 0; i < count; ++i) {
                auto index = readLeb(module.binary, pos);
                auto name = readStr(module.binary, pos);
                printf("        %d %s\n", index, std::string{name}.c_str());
                check(index < module.functions.size(),
                      "invalid function index in name");
            }
        } else {
            pos = subEnd;
        }
    }
    check(pos == sEnd, "name section malformed");
}

inline void read_reloc(Module& module, std::string_view name, size_t& pos,
                       size_t sEnd) {
    printf("%s\n", std::string{name}.c_str());
    auto section_id = readLeb(module.binary, pos);
    check(section_id == wasm_sec_code || section_id == wasm_sec_data,
          "unsupported reloc section id");
    auto count = readLeb(module.binary, pos);
    printf("    %d relocs\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        auto type = readLeb(module.binary, pos);
        auto offset = readLeb(module.binary, pos);
        auto index = readLeb(module.binary, pos);
        auto addend = uint32_t{0};
        if (type == r_webassembly_memory_addr_leb ||
            type == r_webassembly_memory_addr_sleb ||
            type == r_webassembly_memory_addr_i32)
            addend = readLeb(module.binary, pos);
        module.relocs.push_back({section_id, type, offset, index, addend});
    }
    check(pos == sEnd, "reloc section malformed");
}

inline void read_linking(Module& module, size_t& pos, size_t sEnd) {
    printf("linking\n");
    while (pos < sEnd) {
        auto type = module.binary[pos++];
        readLeb(module.binary, pos); // len
        if (type == wasm_symbol_info) {
            auto count = readLeb(module.binary, pos);
            for (uint32_t i = 0; i < count; ++i) {
                auto name = readStr(module.binary, pos);
                auto flags = readLeb(module.binary, pos);
                printf("    symbol  %s flags=%d\n", std::string{name}.c_str(),
                       flags);
                auto& symbol = module.symbols[name];
                symbol.flags = flags;
                symbol.inLinking = true;
            }
        } else if (type == wasm_data_size) {
            module.dataSize = readLeb(module.binary, pos);
            printf("    dataSize: %d\n", module.dataSize);
        } else if (type == wasm_segment_info) {
            auto count = readLeb(module.binary, pos);
            for (uint32_t i = 0; i < count; ++i) {
                auto name = readStr(module.binary, pos);
                auto alignment = readLeb(module.binary, pos);
                auto flags = readLeb(module.binary, pos);
                printf("    segment %s alignment=%d flags=%d\n",
                       std::string{name}.c_str(), alignment, flags);
            }
        } else {
            check(false,
                  "unhandled linking subsection " + std::to_string(type));
        }
    }
    check(pos == sEnd, "linking section malformed");
    for (auto& [name, symbol] : module.symbols)
        if (!symbol.inLinking && !symbol.exports.empty())
            check(false, "symbol " + std::string{name} +
                             " is exported, but not in linking section");
}

inline Module read_module(std::vector<uint8_t> bin) {
    auto module = Module{std::move(bin)};
    check(module.binary.size() >= 8 &&
              *(uint32_t*)(&module.binary[0]) == 0x6d736100,
          "not a wasm file");
    auto pos = size_t{8};
    auto end = module.binary.size();
    while (pos != end) {
        auto id = module.binary[pos++];
        check(id < num_sections, "invalid section id");
        auto payloadLen = readLeb(module.binary, pos);
        auto sEnd = pos + payloadLen;
        check(sEnd <= module.binary.size(), "section extends past file end");
        if (id) {
            module.sections[id] = {true, pos, sEnd};
            switch (id) {
            case wasm_sec_type:
                read_sec_type(module, pos, sEnd);
                break;
            case wasm_sec_import:
                read_sec_import(module, pos, sEnd);
                break;
            case wasm_sec_function:
                read_sec_function(module, pos, sEnd);
                break;
            case wasm_sec_table:
                read_sec_table(module, pos, sEnd);
                break;
            case wasm_sec_memory:
                read_sec_memory(module, pos, sEnd);
                break;
            case wasm_sec_global:
                read_sec_global(module, pos, sEnd);
                break;
            case wasm_sec_export:
                read_sec_export(module, pos, sEnd);
                break;
            case wasm_sec_start:
                read_sec_start(module, pos, sEnd);
                break;
            case wasm_sec_elem:
                read_sec_elem(module, pos, sEnd);
                break;
            case wasm_sec_code:
                read_sec_code(module, pos, sEnd);
                break;
            case wasm_sec_data:
                read_sec_data(module, pos, sEnd);
                break;
            default:
                check(false, "unknown section id");
            }
        } else {
            auto name = readStr(module.binary, pos);
            if (name.starts_with("reloc."))
                read_reloc(module, name, pos, sEnd);
            else if (name == "linking")
                read_linking(module, pos, sEnd);
        }
        pos = sEnd;
    }
    return module;
} // read_module

} // namespace WasmTools
