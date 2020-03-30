// Context.h

#ifndef CONTEXT_H
#define CONTEXT_H

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

class CodeSection;
class CustomSection;
class DataCountSection;
class DataSection;
class ElementDeclaration;
class ElementSection;
class Event;
class EventSection;
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

        auto nextEventCount()
        {
            return eventCount++;
        }

        auto getEventCount() const
        {
            return eventCount;
        }

        void addEvent(Event* mwmory)
        {
            eventTable.push_back(mwmory);
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

        void addImport(ImportDeclaration* symbol)
        {
            importTable.push_back(symbol);
        }

        ImportDeclaration* getImport(uint32_t index) const
        {
            return importTable[index];
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
        void generate(std::ostream& os);

        void makeDataCountSection();

    protected:
        uint32_t codeCount = 0;
        uint32_t elementCount = 0;
        uint32_t eventCount = 0;
        uint32_t exportCount = 0;
        uint32_t functionCount = 0;
        uint32_t globalCount = 0;
        uint32_t localFunctionStart = 0;
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

        std::vector<Global*> globalTable;
        std::vector<ImportDeclaration*> importTable;
        std::vector<Memory*> memoryTable;
        std::vector<Event*> eventTable;
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
        IndexMap eventMap;
        IndexMap tableMap;
        IndexMap typeMap;

        DataBuffer dataBuffer;
        std::vector<IndexMap> localMaps;
        std::vector<uint32_t> localCounts;

        void showSections(std::ostream& os, unsigned flags);
        void generateSections(std::ostream& os);
};

class BinaryContext : public Context
{
    public:
        BinaryContext(BinaryErrorHandler& error)
          : errorHandler(error)
        {
        }

        BinaryContext(Context& other, BinaryErrorHandler& error)
          : Context(other), errorHandler(error)
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
        void write(std::ostream& os);

        template<typename T, typename... Ts>
        T* makeTreeNode(const Ts&... ts)
        {
            T* result = new T(ts...);

            result->setColumnNumber(dataBuffer.getPos());
            return result;
        }

    private:
        BinaryErrorHandler& errorHandler;

        void dumpSections(std::ostream& os);
        void writeHeader();
        void writeSections();
        void writeFile(std::ostream& os);

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

        void write(std::ostream& os);

        template<typename T, typename... Ts>
        T* makeTreeNode(const Ts&... ts)
        {
            T* result = new T(ts...);

            result->setLineNumber(tokenBuffer.peekToken(-1).getLineNumber());
            result->setColumnNumber(tokenBuffer.peekToken(-1).getColumnNumber());
            return result;
        }

    private:
        TokenBuffer& tokenBuffer;
        SourceErrorHandler& errorHandler;

        friend class Assembler;
};

class CheckContext : public Context
{
    public:
        CheckContext(CheckErrorHandler& error)
          : errorHandler(error)
        {
        }

        CheckContext(Context& other, CheckErrorHandler& error)
          : Context(other), errorHandler(error)
        {
        }

        auto& msgs()
        {
            return errorHandler;
        }

        bool checkSemantics();
        void checkDataCount(TreeNode* node, uint32_t count);
        void checkTypeIndex(TreeNode* node, uint32_t count);
        void checkFunctionIndex(TreeNode* node, uint32_t index);
        void checkMemoryIndex(TreeNode* node, uint32_t index);
        void checkTableIndex(TreeNode* node, uint32_t index);
        void checkGlobalIndex(TreeNode* node, uint32_t index);
        void checkValueType(TreeNode* node, const ValueType& type);
        void checkElementType(TreeNode* node, const ValueType& type);
        void checkExternalType(TreeNode* node, const ExternalType& type);
        void checkEventType(TreeNode* node, const EventType& type);
        void checkLimits(TreeNode* node, const Limits& limits);
        void checkMut(TreeNode* node, Mut& mut);

    private:
        CheckErrorHandler& errorHandler;
};

#endif
