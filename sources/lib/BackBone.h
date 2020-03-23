// BackBone.h

#ifndef BACKBONE_H
#define BACKBONE_H

#include "Context.h"
#include "DataBuffer.h"
#include "Encodings.h"
#include "Instruction.h"
#include "TokenBuffer.h"

#include <iostream>
#include <memory>
#include <vector>

class Expression
{
    public:
        void addInstruction(Instruction* instruction)
        {
            instructions.emplace_back(instruction);
        }

        auto& getInstructions()
        {
            return instructions;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static Expression* parse(SourceContext& context, bool oneInstruction = false);
        static Expression* read(BinaryContext& context);
        static Expression* read(BinaryContext& context, size_t endPos);
        static Expression* readInit(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<Instruction>> instructions;
};

class Section
{
    public:
        Section(SectionType t)
          : type(t)
        {
        }

        virtual ~Section() = default;

        void setOffsets(size_t start, size_t end)
        {
            startOffset = start;
            endOffset = end;
        }

        auto getType() const
        {
            return type;
        }

        auto getStartOffset() const
        {
            return startOffset;
        }

        auto getEndOffset() const
        {
            return endOffset;
        }

        void dump(std::ostream& os, BinaryContext& context);

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) = 0;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) = 0;
        virtual void write(BinaryContext& context) const = 0;

    protected:
        size_t startOffset = 0;
        size_t endOffset = 0;
        SectionType type = SectionType::custom;
};

class CustomSection : public Section
{
    public:
        CustomSection()
          : Section(SectionType::custom)
        {
        }

        std::string_view getName() const
        {
            return name;
        }

        void setName(std::string_view value)
        {
            name = value;
        }

        virtual void write(BinaryContext& context) const override
        {
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;

        static CustomSection* read(BinaryContext& context);

    protected:
        std::string name;
};

class RelocationEntry
{
    public:
        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);

        static RelocationEntry* read(BinaryContext& context);

        auto getType() const
        {
            return type;
        }

        void setType(RelocationType value)
        {
            type = value;
        }

        auto getOffset() const
        {
            return offset;
        }

        void setOffset(uint32_t value)
        {
            offset = value;
        }

        auto getIndex() const
        {
            return index;
        }

        void setIndex(uint32_t value)
        {
            index = value;
        }

        auto getAddend() const
        {
            return addend;
        }

        void setAddend(int32_t value)
        {
            addend = value;
        }

    private:
        RelocationType type;
        uint32_t offset = 0;
        uint32_t index = invalidIndex;
        int32_t addend = 0;
};

class RelocationSection : public CustomSection
{
    public:
        auto getTargetSectionIndex() const
        {
            return targetSectionIndex;
        }

        void setTargetSectionIndex(uint32_t value)
        {
            targetSectionIndex = value;
        }

        void addRelocation(RelocationEntry* entry)
        {
            relocations.emplace_back(entry);
        }

        auto& getRelocations()
        {
            return relocations;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override
        {
        }

        static RelocationSection* read(BinaryContext& context);

    private:
        uint32_t targetSectionIndex = invalidIndex;
        std::vector<std::unique_ptr<RelocationEntry>> relocations;
};

class LinkingSubsection
{
    public:
        auto getType() const
        {
            return type;
        }

        void setType(LinkingType value)
        {
            type = value;
        }

        virtual ~LinkingSubsection() = default;

        virtual void show(std::ostream& os, Context& context);
        virtual void generate(std::ostream& os, Context& context);

        static LinkingSubsection* read(BinaryContext& context);

    protected:
        LinkingType type;
};

class LinkingSegmentInfo
{
    public:
        std::string_view getName() const
        {
            return name;
        }

        void setName(std::string_view value)
        {
            name = value;
        }

        auto getAlign() const
        {
            return align;
        }

        void setAlign(uint32_t value)
        {
            align = value;
        }

        auto getFlags() const
        {
            return flags;
        }

        void setFlags(uint32_t value)
        {
            flags = value;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);

        static LinkingSegmentInfo* read(BinaryContext& context);

    private:
        std::string name;
        uint32_t align = 0;
        uint32_t flags = 0;
};

class LinkingSegmentSubsection : public LinkingSubsection
{
    public:
        auto& getInfos()
        {
            return infos;
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        static LinkingSegmentSubsection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<LinkingSegmentInfo>> infos;
};

class LinkingInitFunc
{
    public:
        auto getPriority() const
        {
            return priority;
        }

        void setPriority(uint32_t value)
        {
            priority = value;
        }

        auto getFunctionIndex() const
        {
            return functionIndex;
        }

        void setFunctionIndex(uint32_t value)
        {
            functionIndex = value;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);

        static LinkingInitFunc* read(BinaryContext& context);

    private:
        uint32_t priority = 0;
        uint32_t functionIndex = invalidIndex;
};

class LinkingInitFuncSubsection : public LinkingSubsection
{
    public:
        auto& getInits()
        {
            return inits;
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        static LinkingInitFuncSubsection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<LinkingInitFunc>> inits;
};

class ComdatSym
{
    public:
        auto getIndex() const
        {
            return index;
        }

        void setIndex(uint32_t value)
        {
            index = value;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);

        static ComdatSym* read(BinaryContext& context);

    private:
        ComdatSymKind kind;
        uint32_t index = invalidIndex;
};

class LinkingComdat
{
    public:
        std::string_view getName() const
        {
            return name;
        }

        void setName(std::string_view value)
        {
            name = value;
        }

        auto getFlags() const
        {
            return flags;
        }

        void setFlags(uint32_t value)
        {
            flags = value;
        }

        auto& getSyms()
        {
            return syms;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);

        static LinkingComdat* read(BinaryContext& context);

    private:
        std::string name;
        uint32_t flags = 0;
        std::vector<std::unique_ptr<ComdatSym>> syms;
};

class LinkingComdatSubsection : public LinkingSubsection
{
    public:
        auto& getComdats()
        {
            return comdats;
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        static LinkingComdatSubsection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<LinkingComdat>> comdats;
};

class SymbolTableInfo
{
    public:
        virtual ~SymbolTableInfo() = default;
        virtual void show(std::ostream& os, Context& context);
        virtual void generate(std::ostream& os, Context& context);

        virtual std::string_view getName() const
        {
            return {};
        }

        virtual uint32_t getIndex() const
        {
            return ~uint32_t(0);
        }

        void showFlags(std::ostream& os);

        static SymbolTableInfo* read(BinaryContext& context);

    protected:
        SymbolKind kind;
        SymbolFlags flags;
};

class SymbolTableFGETInfo : public SymbolTableInfo
{
    public:
        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        virtual std::string_view getName() const override
        {
            return name;
        }

        void setName(std::string_view value)
        {
            name = value;
        }

        virtual uint32_t getIndex() const override
        {
            return index;
        }

        void setIndex(uint32_t value)
        {
            index = value;
        }

        static SymbolTableFGETInfo* read(BinaryContext& context, SymbolKind kind, SymbolFlags flags);

    protected:
        uint32_t index = invalidIndex;
        std::string name;
};

class SymbolTableDataInfo : public SymbolTableInfo
{
    public:
        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        virtual std::string_view getName() const override
        {
            return name;
        }

        void setName(std::string_view value)
        {
            name = value;
        }

        virtual uint32_t getIndex() const override
        {
            return dataIndex;
        }

        void setIndex(uint32_t value)
        {
            dataIndex = value;
        }

        auto getOffset() const
        {
            return offset;
        }

        void setOffset(uint32_t value)
        {
            offset = value;
        }

        auto getSize() const
        {
            return size;
        }

        void setSize(uint32_t value)
        {
            size = value;
        }

        static SymbolTableDataInfo* read(BinaryContext& context, SymbolFlags flags);

    protected:
        std::string name;
        uint32_t dataIndex = invalidIndex;
        uint32_t offset = 0;
        uint32_t size = 0;
};

class SymbolTableSectionInfo : public SymbolTableInfo
{
    public:
        virtual uint32_t getIndex() const override
        {
            return tableIndex;
        }

        void setIndex(uint32_t value)
        {
            tableIndex = value;
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        static SymbolTableSectionInfo* read(BinaryContext& context, SymbolFlags flags);

    protected:
        uint32_t tableIndex = invalidIndex;
};

class LinkingSymbolTableSubSectionn : public LinkingSubsection
{
    public:
        auto& getInfos()
        {
            return infos;
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;

        static LinkingSymbolTableSubSectionn* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<SymbolTableInfo>> infos;
};

class LinkingSection : public CustomSection
{
    public:
        auto getVersion() const
        {
            return version;
        }

        void setVersion(uint32_t value)
        {
            version = value;
        }

        auto& getSubSections()
        {
            return subSections;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override
        {
        }

        static LinkingSection* read(BinaryContext& context, size_t endPos);

    private:
        uint32_t version = 0;
        std::vector<std::unique_ptr<LinkingSubsection>> subSections;
};

class Local
{
    public:
        Local() = default;
        Local(ValueType t)
            : type(t)
        {
        }

        Local(std::string_view i, ValueType v)
          : id(i), type(v)
        {
        }

        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto getType() const
        {
            return type;
        }

        void setNumber(uint32_t n)
        {
            number = n;
        }

        auto getNumber() const
        {
            return number;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);

        static Local* parse(SourceContext& context);

    protected:
        std::string id;
        ValueType type = ValueType::void_;
        uint32_t number;

    friend class CodeEntry;
};

class Signature
{
    public:
        Signature() = default;

        Signature(const Signature& other)
        {
            params = other.params;
            results = other.results;
        }

        auto& getParams()
        {
            return params;
        }

        auto& getResults()
        {
            return results;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        bool operator==(const Signature& other);
        bool operator!=(const Signature& other)
        {
            return !operator==(other);
        }

        static Signature* parse(SourceContext& context);
        static Signature* read(BinaryContext& context);

    private:
        std::vector<Local*> params;
        std::vector<ValueType> results;
};

class TypeUse
{
    public:
        auto getSignatureIndex() const
        {
            return signatureIndex;
        }

        void setSignatureIndex(uint32_t s)
        {
            signatureIndex = s;
        }

        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto* getSignature() const
        {
            return signature.get();
        }

        void setSignature(Signature* value)
        {
            signature.reset(value);
        }

        void checkSignature(SourceContext& context);

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static void parse(SourceContext& context, TypeUse* result);
        static void read(BinaryContext& context, TypeUse* result);

    protected:
        uint32_t signatureIndex = invalidIndex;
        std::unique_ptr<Signature> signature;
        std::string id;
};

class TypeDeclaration
{
    public:
        TypeDeclaration() = default;
        TypeDeclaration(Signature* sig)
          : signature(sig)
        {
        }

        auto* getSignature()
        {
            return signature.get();
        }

        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t n)
        {
            number = n;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static TypeDeclaration* parse(SourceContext& context);
        static TypeDeclaration* read(BinaryContext& context);

    private:
        std::unique_ptr<Signature> signature;
        uint32_t number = 0;
        std::string id;
};

class TypeSection : public Section
{
    public:
        TypeSection()
          : Section(SectionType::type)
        {
        }

        void addType(TypeDeclaration* type)
        {
            types.emplace_back(type);
        }

        auto& getTypes()
        {
            return types;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static void read(BinaryContext& context, TypeSection* result);

    private:
        std::vector<std::unique_ptr<TypeDeclaration>> types;
};

class ImportDeclaration
{
    public:
        ImportDeclaration(ExternalKind k)
          : kind(k)
        {
        }

        virtual ~ImportDeclaration() = default;

        const std::string_view getModuleName() const
        {
            return moduleName;
        }

        void setModuleName(std::string_view n)
        {
            moduleName = n;
        }

        const std::string_view getName() const
        {
            return name;
        }

        void setName(std::string_view n)
        {
            name = n;
        }

        std::string_view getName()
        {
            return name;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        auto getNumber() const
        {
            return number;
        }

        auto getKind() const
        {
            return kind;
        }

        void generateNames(std::ostream& os);
        virtual void show(std::ostream& os, Context& context) = 0;
        virtual void generate(std::ostream& os, Context& context) = 0;
        virtual void write(BinaryContext& context) const = 0;

        static ImportDeclaration* parse(SourceContext& context);
        static ImportDeclaration* parseFunctionImport(SourceContext& context);
        static ImportDeclaration* parseTableImport(SourceContext& context);
        static ImportDeclaration* parseMemoryImport(SourceContext& context);
        static ImportDeclaration* parseGlobalImport(SourceContext& context);

    protected:
        std::string moduleName;
        std::string name;
        uint32_t number = 0;
        ExternalKind kind;
};

class FunctionImport : public TypeUse, public ImportDeclaration
{
    public:
        FunctionImport()
          : ImportDeclaration(ExternalKind::function)
        {
        }

        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;
        virtual void write(BinaryContext& context) const override;

        static FunctionImport* parse(SourceContext& context, const std::string_view name);
        static FunctionImport* read(BinaryContext& context, const std::string_view name);
};

class Table
{
    public:
        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto getType() const
        {
            return type;
        }

        void setType(ValueType value)
        {
            type = value;
        }

        auto& getLimits()
        {
            return limits;
        }

        void setLimits(const Limits& l)
        {
            limits = l;
        }

    protected:
        ValueType type = ValueType::void_;
        Limits limits;
        std::string id;
};

class TableImport : public Table, public ImportDeclaration
{
    public:
        TableImport()
          : ImportDeclaration(ExternalKind::table)
        {
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;
        virtual void write(BinaryContext& context) const override;

        static TableImport* parse(SourceContext& context, const std::string_view name);
        static TableImport* read(BinaryContext& context, const std::string_view name);
};

class Memory
{
    public:
        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto& getLimits()
        {
            return limits;
        }

        void setLimits(const Limits& l)
        {
            limits = l;
        }

    protected:
        Limits limits;
        std::string id;
};

class MemoryImport : public Memory, public ImportDeclaration
{
    public:
        MemoryImport()
          : ImportDeclaration(ExternalKind::memory)
        {
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;
        virtual void write(BinaryContext& context) const override;

        static MemoryImport* parse(SourceContext& context, const std::string_view name);
        static MemoryImport* read(BinaryContext& context, const std::string_view name);
};

class Global
{
    public:
        auto getType() const
        {
            return type;
        }

        void setType(ValueType t)
        {
            type = t;
        }

        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto getMut() const
        {
            return mut;
        }

        void setMut (Mut m)
        {
            mut = m;
        }

    protected:
        ValueType type = ValueType::void_;
        Mut mut;
        std::string id;
};

class GlobalImport : public Global, public ImportDeclaration
{
    public:
        GlobalImport()
          : ImportDeclaration(ExternalKind::global)
        {
        }

        virtual void show(std::ostream& os, Context& context) override;
        virtual void generate(std::ostream& os, Context& context) override;
        virtual void write(BinaryContext& context) const override;

        static GlobalImport* parse(SourceContext& context, const std::string_view name);
        static GlobalImport* read(BinaryContext& context, const std::string_view name);
};

class ImportSection : public Section
{
    public:
        ImportSection()
          : Section(SectionType::import)
        {
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static ImportSection* read(BinaryContext& context);

        auto& getImports() const
        {
            return imports;
        }

        void addImport(ImportDeclaration* import)
        {
            imports.emplace_back(import);
        }

    private:
        std::vector<std::unique_ptr<ImportDeclaration>> imports;
};

class CodeEntry;
class FunctionDeclaration : public TypeUse
{
    public:
        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static FunctionDeclaration* parse(SourceContext& context, CodeEntry*& codeEntry);
        static FunctionDeclaration* read(BinaryContext& context);

    protected:
        uint32_t number = 0;
};

class FunctionSection : public Section
{
    public:
        FunctionSection()
          : Section(SectionType::function)
        {
        }

        auto& getFunctions() const
        {
            return functions;
        }

        void addFunction(FunctionDeclaration* function)
        {
            functions.emplace_back(function);
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static FunctionSection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<FunctionDeclaration>> functions;
};

class TableDeclaration : public Table
{
    public:
        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static TableDeclaration* parse(SourceContext& context);
        static TableDeclaration* read(BinaryContext& context);

    private:
        uint32_t number = 0;
};

class TableSection : public Section
{
    public:
        TableSection()
          : Section(SectionType::table)
        {
        }

        void addTable(TableDeclaration* table)
        {
            tables.emplace_back(table);
        }

        auto& getTables()
        {
            return tables;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static TableSection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<TableDeclaration>> tables;
};

class MemoryDeclaration : public Memory
{
    public:
        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static MemoryDeclaration* parse(SourceContext& context);
        static MemoryDeclaration* read(BinaryContext& context);

    private:
        uint32_t number = 0;
};

class MemorySection : public Section
{
    public:
        MemorySection()
          : Section(SectionType::memory)
        {
        }

        void addMemory(MemoryDeclaration* memory)
        {
            memories.emplace_back(memory);
        }

        auto& getMemories()
        {
            return memories;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static MemorySection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<MemoryDeclaration>> memories;
};

class GlobalDeclaration : public Global
{
    public:
        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static GlobalDeclaration* parse(SourceContext& context);
        static GlobalDeclaration* read(BinaryContext& context);

    private:
        std::unique_ptr<Expression> expression;
        uint32_t number = 0;
};

class GlobalSection : public Section
{
    public:
        GlobalSection()
          : Section(SectionType::global)
        {
        }

        void addGlobal(GlobalDeclaration* global)
        {
            globals.emplace_back(global);
        }

        auto& getGlobals()
        {
            return globals;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static GlobalSection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<GlobalDeclaration>> globals;
};

class ExportDeclaration
{
    public:
        std::string_view getName() const
        {
            return name;
        }

        void setName(std::string_view value)
        {
            name = value;
        }

        auto getIndex() const
        {
            return index;
        }

        void setIndex(uint32_t value)
        {
            index = value;
        }

        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        auto getKind() const
        {
            return kind;
        }

        void setKind(ExternalKind k)
        {
            kind = k;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static ExportDeclaration* parse(SourceContext& context);
        static ExportDeclaration* read(BinaryContext& context);

    private:
        std::string name;
        ExternalKind kind;
        uint32_t index = invalidIndex;
        uint32_t number = 0;
};

class ExportSection : public Section
{
    public:
        ExportSection()
          : Section(SectionType::export_)
        {
        }

        void addExport(ExportDeclaration* _export)
        {
            exports.emplace_back(_export);
        }

        auto& getExports()
        {
            return exports;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static ExportSection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<ExportDeclaration>> exports;
};

class StartSection : public Section
{
    public:
        StartSection()
          : Section(SectionType::start)
        {
        }

        auto getFunctionIndex() const
        {
            return functionIndex;
        }

        void setFunctionIndex(uint32_t value)
        {
            functionIndex = value;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static StartSection* parse(SourceContext& context);
        static StartSection* read(BinaryContext& context);

    private:
        uint32_t functionIndex = invalidIndex;
};

class ElementDeclaration
{
    public:
        auto getTableIndex() const
        {
            return tableIndex;
        }

        void setTableIndex(uint32_t value)
        {
            tableIndex = value;
        }

        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        void addFunctionIndex(uint32_t i)
        {
            functionIndexes.push_back(i);
        }

        auto& getFunctionIndexes()
        {
            return functionIndexes;
        }

        auto* getExpression() const
        {
            return expression.get();
        }

        void setExpression(Expression* value)
        {
            expression.reset(value);
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static ElementDeclaration* parse(SourceContext& context);
        static ElementDeclaration* read(BinaryContext& context);

    private:
        uint32_t tableIndex = 0;
        std::unique_ptr<Expression> expression;
        std::vector<uint32_t> functionIndexes;
        uint32_t number = 0;
};

class ElementSection : public Section
{
    public:
        ElementSection()
          : Section(SectionType::element)
        {
        }

        auto& getElements() const
        {
            return elements;
        }

        void addElement(ElementDeclaration* element)
        {
            elements.emplace_back(element);
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static ElementSection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<ElementDeclaration>> elements;
};

class CodeEntry
{
    public:
        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        auto& getLocals()
        {
            return locals;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static CodeEntry* parse(SourceContext& context);
        static CodeEntry* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<Local>> locals;
        std::unique_ptr<Expression> expression;
        uint32_t number = 0;
};

class CodeSection : public Section
{
    public:
        CodeSection()
          : Section(SectionType::code)
        {
        }

        void addCode(CodeEntry* code)
        {
            codes.emplace_back(code);
        }

        auto& getCodes()
        {
            return codes;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static CodeSection* read(BinaryContext& context);

    private:
        std::vector<std::unique_ptr<CodeEntry>> codes;
};

class DataSegment
{
    public:
        auto getNumber() const
        {
            return number;
        }

        void setNumber(uint32_t i)
        {
            number = i;
        }

        auto getMemoryIndex() const
        {
            return memoryIndex;
        }

        void setMemoryIndex(uint32_t value)
        {
            memoryIndex = value;
        }

        std::string_view getInit() const
        {
            return init;
        }

        void setInit(std::string_view value)
        {
            init = value;
        }

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void write(BinaryContext& context) const;

        static DataSegment* parse(SourceContext& context);
        static DataSegment* read(BinaryContext& context);

    private:
        uint32_t memoryIndex = 0;
        uint32_t number = 0;
        std::unique_ptr<Expression> expression;
        std::string init;
};

class DataSection : public Section
{
    public:
        DataSection()
          : Section(SectionType::data)
        {
        }

        auto& getSegments() const
        {
            return segments;
        }

        void addSegment(DataSegment* segment)
        {
            segments.emplace_back(segment);
        }

        auto& getSegments()
        {
            return segments;
        }

        static DataSection* read(BinaryContext& context);

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

    private:
        std::vector<std::unique_ptr<DataSegment>> segments;
};

class DataCountSection : public Section
{
    public:
        DataCountSection()
          : Section(SectionType::dataCount)
        {
        }

        auto getDataCount() const
        {
            return dataCount;
        }

        void setDataCount(uint32_t value)
        {
            dataCount = value;
        }

        virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
        virtual void write(BinaryContext& context) const override;

        static DataCountSection* read(BinaryContext& context);

    private:
        uint32_t dataCount = 0;
};

#endif
