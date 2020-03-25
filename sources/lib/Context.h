// Context.h

#ifndef CONTEXT_H
#define CONTEXT_H

#include "ErrorHandler.h"
#include "common.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

class CodeSection;
class CustomSection;
class DataBuffer;
class DataCountSection;
class DataSection;
class ElementDeclaration;
class ElementSection;
class ExportDeclaration;
class ExportSection;
class FunctionSection;
class Global;
class GlobalSection;
class ImportDeclaration;
class ImportSection;
class InstructionFunctionIdx;
class Local;
class Memory;
class MemorySection;
class Section;
class StartSection;
class SymbolTableInfo;
class Table;
class TableSection;
class TokenBuffer;
class TypeSection;
class TypeUse;
class TypeDeclaration;

class IndexMap
{
    public:
        bool add(std::string_view id, uint32_t index);

        uint32_t getIndex(std::string_view id) const;
        void clear()
        {
            entries.clear();
        }

    private:
        struct Entry
        {
            Entry(std::string_view i, uint32_t j)
              : id(i), index(j)
            {
            }

            bool operator<(const Entry& other) const
            {
                return id < other.id;
            }

            std::string id;
            uint32_t index;
        };

        std::vector<Entry> entries;
};

class Context
{
    public:
        Context();
        ~Context();
        Context(const Context& other);

        auto& getSections()
        {
            return sections;
        }

        void setFunctionInfo(uint32_t index, std::string_view name, uint32_t signatureIndex);
        void setFunctionName(uint32_t index, std::string_view name);

        void addSymbol(SymbolTableInfo* symbol)
        {
            symbolTable.push_back(symbol);
        }

        SymbolTableInfo* getSymbol(uint32_t index) const
        {
            return symbolTable[index];
        }

        uint32_t getTypeCount() const;
        TypeDeclaration* getType(uint32_t index) const;

        bool addTypeId(std::string_view id, uint32_t index)
        {
            return typeMap.add(id, index);
        }

        uint32_t getTypeIndex(std::string_view id)
        {
            return typeMap.getIndex(id);
        }

        auto nextFunctionCount()
        {
            return functionCount++;
        }

        auto getFunctionCount() const
        {
            return functionCount;
        }

        void addFunction(TypeUse* function)
        {
            functionTable.push_back(function);
        }

        bool addFunctionId(std::string_view id, uint32_t index)
        {
            return functionMap.add(id, index);
        }

        TypeUse* getFunction(uint32_t index) const
        {
            return functionTable[index];
        }

        uint32_t getFunctionIndex(std::string_view id)
        {
            return functionMap.getIndex(id);
        }

        auto nextTableCount()
        {
            return tableCount++;
        }

        auto getTableCount() const
        {
            return tableCount;
        }

        void addTable(Table* table)
        {
            tableTable.push_back(table);
        }

        bool addTableId(std::string_view id, uint32_t index)
        {
            return tableMap.add(id, index);
        }

        Table* getTable(uint32_t index) const
        {
            return tableTable[index];
        }

        uint32_t getTableIndex(std::string_view id)
        {
            return tableMap.getIndex(id);
        }

        auto nextMemoryCount()
        {
            return memoryCount++;
        }

        auto getMemoryCount() const
        {
            return memoryCount;
        }

        void addMemory(Memory* mwmory)
        {
            memoryTable.push_back(mwmory);
        }

        bool addMemoryId(std::string_view id, uint32_t index)
        {
            return memoryMap.add(id, index);
        }

        Memory* getMemory(uint32_t index) const
        {
            return memoryTable[index];
        }

        uint32_t getMemoryIndex(std::string_view id)
        {
            return memoryMap.getIndex(id);
        }

        auto nextGlobalCount()
        {
            return globalCount++;
        }

        auto getGlobalCount() const
        {
            return globalCount;
        }

        void addGlobal(Global* global)
        {
            globalTable.push_back(global);
        }

        bool addGlobalId(std::string_view id, uint32_t index)
        {
            return globalMap.add(id, index);
        }

        Global* getGlobal(uint32_t index) const
        {
            return globalTable[index];
        }

        uint32_t getGlobalIndex(std::string_view id)
        {
            return globalMap.getIndex(id);
        }

        void addImport(ImportDeclaration* symbol)
        {
            importTable.push_back(symbol);
        }

        ImportDeclaration* getImport(uint32_t index) const
        {
            return importTable[index];
        }

        auto nextEventCount()
        {
            return eventCount++;
        }

        auto getEventCount() const
        {
            return eventCount;
        }

        auto nextExportCount()
        {
            return exportCount++;
        }

        auto getExportCount() const
        {
            return exportCount;
        }

        auto nextCodeCount()
        {
            return codeCount++;
        }

        auto getCodeCount() const
        {
            return codeCount;
        }

        auto nextSegmentCount()
        {
            return segmentCount++;
        }

        auto getSegmentCount() const
        {
            return segmentCount;
        }

        auto nextElementCount()
        {
            return elementCount++;
        }

        auto getElementCount() const
        {
            return elementCount;
        }

        void startLocalFunctions()
        {
            localFunctionStart = functionCount;
            codeCount = functionCount;
        }

        auto getLocalFunctionStart() const
        {
            return localFunctionStart;
        }

        Section* getSection(size_t index) const
        {
            assert(index < sections.size());
            return sections[index].get();
        }

        TypeSection* getTypeSection() const
        {
            if (typeSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<TypeSection*>(sections[typeSectionIndex].get());
            }
        }

        ImportSection* getImportSection() const
        {
            if (importSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<ImportSection*>(sections[importSectionIndex].get());
            }
        }

        MemorySection* getMemorySection() const
        {
            if (memorySectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<MemorySection*>(sections[memorySectionIndex].get());
            }
        }

        TableSection* getTableSection() const
        {
            if (tableSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<TableSection*>(sections[tableSectionIndex].get());
            }
        }

        ElementSection* getElementSection() const
        {
            if (elementSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<ElementSection*>(sections[elementSectionIndex].get());
            }
        }

        GlobalSection* getGlobalSection() const
        {
            if (globalSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<GlobalSection*>(sections[globalSectionIndex].get());
            }
        }

        ExportSection* getExportSection() const
        {
            if (exportSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<ExportSection*>(sections[exportSectionIndex].get());
            }
        }

        FunctionSection* getFunctionSection() const
        {
            if (functionSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<FunctionSection*>(sections[functionSectionIndex].get());
            }
        }

        CodeSection* getCodeSection() const
        {
            if (codeSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<CodeSection*>(sections[codeSectionIndex].get());
            }
        }

        DataSection* getDataSection() const
        {
            if (dataSectionIndex == invalidSection) {
                return nullptr;
            } else {
                return reinterpret_cast<DataSection*>(sections[dataSectionIndex].get());
            }
        }

        void startFunction();
        void endFunction();
        void startCode(uint32_t number);

        bool addLocalId(std::string_view id, uint32_t index)
        {
            return localMap.add(id, index);
        }

        auto nextLocalCount()
        {
            return localCount++;
        }

        auto getLocalCount() const
        {
            return localCount;
        }

        uint32_t getLocalIndex(std::string_view id)
        {
            return localMap.getIndex(id);
        }

        void setTypeSectionIndex(size_t index)
        {
            typeSectionIndex = index;
        }

        auto getDataCountSectionIndex() const
        {
            return dataCountSectionIndex;
        }

        auto getDataSectionIndex() const
        {
            return dataSectionIndex;
        }

        void setDataCountSectionIndex(size_t index)
        {
            dataCountSectionIndex = index;
        }

        auto getElementSectionIndex() const
        {
            return elementSectionIndex;
        }

        void setElementSectionIndex(size_t index)
        {
            elementSectionIndex = index;
        }

        auto getExportSectionIndex() const
        {
            return exportSectionIndex;
        }

        void setExportSectionIndex(size_t index)
        {
            exportSectionIndex = index;
        }

        uint32_t getLabelCount() const
        {
            return uint32_t(labelStack.size());
        }

        void pushLabel(std::string_view id)
        {
            labelStack.push_back(id);
        }

        void popLabel()
        {
            labelStack.pop_back();
        }

        uint32_t getLabelIndex(std::string_view id)
        {
            uint32_t count = uint32_t(labelStack.size());

            for (uint32_t i = count; i-- > 0; ) {
                if (labelStack[i] == id) {
                    return count - i - 1;
                }
            }

            return invalidIndex;
        }

        void addExport(ExportDeclaration* _export);
        void addElement(ElementDeclaration* element);

        void show(std::ostream& os, unsigned flags);
        void generate(std::ostream& os, unsigned flags);

    protected:
        uint32_t codeCount = 0;
        uint32_t elementCount = 0;
        uint32_t eventCount = 0;
        uint32_t exportCount = 0;
        uint32_t functionCount = 0;
        uint32_t globalCount = 0;
        uint32_t localFunctionStart = 0;
        uint32_t memoryCount = 0;
        uint32_t segmentCount = 0;
        uint32_t tableCount = 0;
        uint32_t localCount = 0;

        size_t codeSectionIndex = invalidSection;
        size_t dataCountSectionIndex = invalidSection;
        size_t dataSectionIndex = invalidSection;
        size_t elementSectionIndex = invalidSection;
        size_t exportSectionIndex = invalidSection;
        size_t functionSectionIndex = invalidSection;
        size_t globalSectionIndex = invalidSection;
        size_t importSectionIndex = invalidSection;
        size_t memorySectionIndex = invalidSection;
        size_t startSectionIndex = invalidSection;
        size_t tableSectionIndex = invalidSection;
        size_t typeSectionIndex = invalidSection;

        std::vector<Global*> globalTable;
        std::vector<ImportDeclaration*> importTable;
        std::vector<Memory*> memoryTable;
        std::vector<SymbolTableInfo*> symbolTable;
        std::vector<Table*> tableTable;
        std::vector<TypeUse*> functionTable;
        std::vector<size_t> customSectionIndexes;

        std::vector<std::shared_ptr<Section>> sections;
        std::vector<std::string_view> labelStack;
        std::vector<Local*> locals;

        IndexMap functionMap;
        IndexMap globalMap;
        IndexMap localMap;
        IndexMap memoryMap;
        IndexMap tableMap;
        IndexMap typeMap;

        std::vector<IndexMap> localMaps;
        std::vector<uint32_t> localCounts;

        void showSections(std::ostream& os, unsigned flags);
        void generateSections(std::ostream& os, unsigned flags = 0);
};

class BinaryContext : public Context
{
    public:
        BinaryContext(DataBuffer& data, BinaryErrorHandler& error)
          : dataBuffer(data), errorHandler(error)
        {
        }

        BinaryContext(Context& other, DataBuffer& data, BinaryErrorHandler& error)
          : Context(other), dataBuffer(data), errorHandler(error)
        {
        }

        DataBuffer& data()
        {
            return dataBuffer;
        }

        BinaryErrorHandler& msgs()
        {
            return errorHandler;
        }

        void dump(std::ostream& os);

    private:
        DataBuffer& dataBuffer;
        BinaryErrorHandler& errorHandler;

        void dumpSections(std::ostream& os);

        friend class Assembler;
        friend class Disassembler;
};

class SourceContext : public Context
{
    public:
        SourceContext(TokenBuffer& data, SourceErrorHandler& error)
          : tokenBuffer(data), errorHandler(error)
        {
        }

        auto& tokens()
        {
            return tokenBuffer;
        }

        auto& msgs()
        {
            return errorHandler;
        }

    private:
        TokenBuffer& tokenBuffer;
        SourceErrorHandler& errorHandler;

        friend class Assembler;
};

#endif
