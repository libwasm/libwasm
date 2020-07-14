// Module.h

#ifndef MODULE_H
#define MODULE_H

#include "ErrorHandler.h"
#include "common.h"
#include "DataBuffer.h"
#include "Encodings.h"
#include "TokenBuffer.h"
#include "TreeNode.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace libwasm
{
class CodeSection;
class CustomSection;
class DataCountSection;
class DataSection;
class ElementDeclaration;
class ElementSection;
class ElementDeclaration;
class Event;
class EventSection;
class ExportDeclaration;
class ExportSection;
class FunctionSection;
class Global;
class GlobalSection;
class ImportDeclaration;
class ImportSection;
class Instruction;
class InstructionFunctionIdx;
class Local;
class Memory;
class MemorySection;
class Section;
class StartSection;
class SymbolTableInfo;
class Table;
class TableSection;
class TypeSection;
class TypeUse;
class TypeDeclaration;
class FunctionDeclaration;
class TableDeclaration;
class MemoryDeclaration;
class GlobalDeclaration;
class DataSegment;
class CodeEntry;

class Module
{
    protected:
        class IndexMap
        {
            public:
                bool add(std::string_view id, uint32_t index, bool replace = false);

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

    public:
        struct Statistics
        {
            uint32_t elementCount = 0;
            uint32_t eventCount = 0;
            uint32_t exportCount = 0;
            uint32_t functionCount = 0;
            uint32_t globalCount = 0;
            uint32_t importCount = 0;
            uint32_t memoryCount = 0;
            uint32_t tableCount = 0;
            uint32_t typeCount = 0;
            uint32_t dataSegmentCount = 0;
            uint32_t sectionCount = 0;
            size_t instructionCount = 0;
            size_t initInstructionCount = 0;

            void show(std::ostream& os, std::string_view indent = "");
        };

        Module() = default;

        auto& getSections()
        {
            return sections;
        }

        Statistics getStatistics();

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
        uint32_t getImportCount() const;
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

        auto nextEventCount()
        {
            return eventCount++;
        }

        auto getEventCount() const
        {
            return eventCount;
        }

        void addEvent(Event* memory)
        {
            eventTable.push_back(memory);
        }

        bool addEventId(std::string_view id, uint32_t index)
        {
            return eventMap.add(id, index);
        }

        Event* getEvent(uint32_t index) const
        {
            return eventTable[index];
        }

        uint32_t getEventIndex(std::string_view id)
        {
            return eventMap.getIndex(id);
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

        void addImport(ImportDeclaration* import)
        {
            importTable.push_back(import);
        }

        ImportDeclaration* getImport(uint32_t index) const
        {
            return importTable[index];
        }

        bool addSegmentId(std::string_view id, uint32_t index)
        {
            return segmentMap.add(id, index, true);
        }

        DataSegment* getSegment(uint32_t index) const;

        uint32_t getSegmentIndex(std::string_view id)
        {
            return segmentMap.getIndex(id);
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

        uint32_t getSegmentCount() const;

        auto nextElementCount()
        {
            return elementCount++;
        }

        auto getElementCount() const
        {
            return elementCount;
        }

        auto getElementSectionIndex() const
        {
            return elementSectionIndex;
        }

        void setElementSectionIndex(size_t index)
        {
            elementSectionIndex = index;
        }

        bool addElementId(std::string_view id, uint32_t index)
        {
            return elementMap.add(id, index);
        }

        ElementDeclaration* getElement(uint32_t index) const;

        uint32_t getElementIndex(std::string_view id)
        {
            return elementMap.getIndex(id);
        }

        void startLocalFunctions()
        {
            importedFunctionCount = functionCount;
            codeCount = functionCount;
        }

        auto getImportedFunctionCount() const
        {
            return importedFunctionCount;
        }

        void startLocalMemories()
        {
            importedMemoryCount = memoryCount;
        }

        auto getImportedMemoryCount() const
        {
            return importedMemoryCount;
        }

        void startLocalTables()
        {
            importedTableCount = tableCount;
        }

        auto getImportedTableCount() const
        {
            return importedTableCount;
        }

        void startLocalEvents()
        {
            importedEventCount = eventCount;
        }

        auto getImportedEventCount() const
        {
            return importedEventCount;
        }

        void startLocalGlobals()
        {
            importedGlobalCount = globalCount;
        }

        auto getImportedGlobalCount() const
        {
            return importedGlobalCount;
        }

        Section* getSection(size_t index) const
        {
            assert(index < sections.size());
            return sections[index].get();
        }

        bool setTypeSection(TypeSection* section);
        bool setImportSection(ImportSection* section);
        bool setMemorySection(MemorySection* section);
        bool setTableSection(TableSection* section);
        bool setElementSection(ElementSection* section);
        bool setGlobalSection(GlobalSection* section);
        bool setExportSection(ExportSection* section);
        bool setFunctionSection(FunctionSection* section);
        bool setCodeSection(CodeSection* section);
        bool setDataSection(DataSection* section);
        bool setDataCountSection(DataCountSection* section);
        bool setStartSection(StartSection* section);
        void addCustomSection(CustomSection* section);

        TypeSection* getTypeSection() const;
        ImportSection* getImportSection() const;
        MemorySection* getMemorySection() const;
        TableSection* getTableSection() const;
        ElementSection* getElementSection() const;
        GlobalSection* getGlobalSection() const;
        ExportSection* getExportSection() const;
        FunctionSection* getFunctionSection() const;
        CodeSection* getCodeSection() const;
        DataSection* getDataSection() const;
        DataCountSection* getDataCountSection() const;
        StartSection* getStartSection() const;

        TypeSection* requiredTypeSection();
        ImportSection* requiredImportSection();
        MemorySection* requiredMemorySection();
        TableSection* requiredTableSection();
        ElementSection* requiredElementSection();
        GlobalSection* requiredGlobalSection();
        ExportSection* requiredExportSection();
        FunctionSection* requiredFunctionSection();
        CodeSection* requiredCodeSection();
        DataSection* requiredDataSection();

        void startFunction();
        void endFunction();
        void endType();
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

        void pushLabel(std::string id)
        {
            labelStack.push_back(std::move(id));
        }

        void popLabel()
        {
            labelStack.pop_back();
        }

        uint32_t getLabelIndex(std::string_view id);

        void addTypeEntry(TypeDeclaration* entry);
        void addImportEntry(ImportDeclaration* entry);
        void addFunctionEntry(FunctionDeclaration* entry);
        void addTableEntry(TableDeclaration* entry);
        void addMemoryEntry(MemoryDeclaration* entry);
        void addGlobalEntry(GlobalDeclaration* entry);
        void addExportEntry(ExportDeclaration* entry);
        void addElementEntry(ElementDeclaration* entry);
        void addDataEntry(DataSegment* entry);
        void addCodeEntry(CodeEntry* entry);

        void show(std::ostream& os, unsigned flags);
        void generate(std::ostream& os);
        void generateS(std::ostream& os);
        void generateC(std::ostream& os, bool optimized = false);

        void makeDataCountSection();

        auto needsDataCount() const
        {
            return dataCountFlag;
        }

        void setDataCountNeeded()
        {
            dataCountFlag = true;
        }

        std::string_view getId() const
        {
            return id;
        }

        void setId(std::string_view value)
        {
            id = value;
        }

        auto getUseExpressionS() const
        {
            return useExpressionS;
        }

        std::string getNamePrefix() const;

    protected:
        bool dataCountFlag = false;
        bool useExpressionS = false;

        uint32_t codeCount = 0;
        uint32_t elementCount = 0;
        uint32_t eventCount = 0;
        uint32_t exportCount = 0;
        uint32_t functionCount = 0;
        uint32_t globalCount = 0;
        uint32_t importedFunctionCount = 0;
        uint32_t importedTableCount = 0;
        uint32_t importedMemoryCount = 0;
        uint32_t importedEventCount = 0;
        uint32_t importedGlobalCount = 0;
        uint32_t memoryCount = 0;
        uint32_t tableCount = 0;
        uint32_t localCount = 0;

        size_t codeSectionIndex = invalidSection;
        size_t dataCountSectionIndex = invalidSection;
        size_t dataSectionIndex = invalidSection;
        size_t elementSectionIndex = invalidSection;
        size_t exportSectionIndex = invalidSection;
        size_t eventSectionIndex = invalidSection;
        size_t functionSectionIndex = invalidSection;
        size_t globalSectionIndex = invalidSection;
        size_t importSectionIndex = invalidSection;
        size_t memorySectionIndex = invalidSection;
        size_t startSectionIndex = invalidSection;
        size_t tableSectionIndex = invalidSection;
        size_t typeSectionIndex = invalidSection;

        std::vector<Event*> eventTable;
        std::vector<Global*> globalTable;
        std::vector<ImportDeclaration*> importTable;
        std::vector<Memory*> memoryTable;
        std::vector<SymbolTableInfo*> symbolTable;
        std::vector<Table*> tableTable;
        std::vector<TypeUse*> functionTable;
        std::vector<size_t> customSectionIndexes;

        std::vector<std::unique_ptr<Section>> sections;
        std::vector<std::string> labelStack;
        std::vector<Local*> locals;

        IndexMap elementMap;
        IndexMap eventMap;
        IndexMap functionMap;
        IndexMap globalMap;
        IndexMap localMap;
        IndexMap memoryMap;
        IndexMap segmentMap;
        IndexMap tableMap;
        IndexMap typeMap;

        DataBuffer dataBuffer;
        std::vector<IndexMap> localMaps;
        std::vector<uint32_t> localCounts;
        std::string id;

        void showSections(std::ostream& os, unsigned flags);
        void generateSections(std::ostream& os);
        void generateInitExpression(std::ostream& os, Instruction* instruction);
        void generateCPreamble(std::ostream& os);
};

};

#endif
