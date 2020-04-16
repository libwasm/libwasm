// Context.cpp

#include "Context.h"
#include "BackBone.h"
#include "Encodings.h"
#include "Module.h"
#include "Validator.h"

#include <algorithm>
#include <iostream>

namespace libwasm
{

Context::Context()
{
    module = new Module;
    modules.emplace_back(module);
}

Context::Context(const Context& other) = default;

void BinaryContext::dumpSections(std::ostream& os)
{
    for (auto& section : module->getSections()) {
        section->dump(os, *this);
    }
}

void BinaryContext::dump(std::ostream& os)
{
    dumpSections(os);
}

void BinaryContext::writeHeader()
{
    dataBuffer.putU32(wasmMagic);
    dataBuffer.putU32(wasmVersion);
}

void BinaryContext::writeSections()
{
    if (auto* section = module->getTypeSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getImportSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getFunctionSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getTableSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getMemorySection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getGlobalSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getExportSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getStartSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getElementSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getDataCountSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getCodeSection(); section != nullptr) {
        section->write(*this);
    }

    if (auto* section = module->getDataSection(); section != nullptr) {
        section->write(*this);
    }
}

void BinaryContext::writeFile(std::ostream& os)
{
    os.write(data().data(), data().size());
}

void BinaryContext::write(std::ostream& os)
{
    dataBuffer.reset();

    writeHeader();
    writeSections();
    writeFile(os);
}

void SourceContext::write(std::ostream& os)
{
    BinaryErrorHandler error;
    BinaryContext bContext(*this, error);

    bContext.data().reset();

    bContext.write(os);
}

bool CheckContext::checkSemantics()
{
    if (module->needsDataCount()) {
        module->makeDataCountSection();
    }

    for (auto& section : module->getSections()) {
        section->check(*this);
    }

    validate(*this);
    return errorHandler.getErrorCount() == 0;
}

void CheckContext::checkDataCount(TreeNode* node, uint32_t count)
{
    errorHandler.errorWhen((count != uint32_t(module->getSegmentCount())), node,
            "Invalid data count ", count, "; expected ", module->getSegmentCount(), '.');
}

void CheckContext::checkTypeIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(module->getTypeCount())), node,
            "Type index (", index, ") is larger then maximum (", module->getTypeCount(), ")");
}

void CheckContext::checkFunctionIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(module->getFunctionCount())), node,
            "Function index (", index, ") is larger then maximum (", module->getFunctionCount(), ")");
}

void CheckContext::checkTableIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(module->getTableCount())), node,
            "Table index (", index, ") is larger then maximum (", module->getTableCount(), ")");
}

void CheckContext::checkMemoryIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(module->getMemoryCount())), node,
            "Memory index (", index, ") is larger then maximum (", module->getMemoryCount(), ")");
}

void CheckContext::checkGlobalIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(module->getGlobalCount())), node,
            "Global index (", index, ") is larger then maximum (", module->getGlobalCount(), ")");
}

void CheckContext::checkEventIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(module->getEventCount())), node,
            "Event index (", index, ") is larger then maximum (", module->getEventCount(), ")");
}

void CheckContext::checkValueType(TreeNode* node, const ValueType& type)
{
    errorHandler.errorWhen((!type.isValid()), node,
            "Invalid valuetype ", int32_t(type));
}

void CheckContext::checkElementType(TreeNode* node, const ValueType& type)
{
    errorHandler.errorWhen(!type.isValidRef(), node,
            "Invalid element type ", int32_t(type));
}

void CheckContext::checkExternalType(TreeNode* node, const ExternalType& type)
{
    errorHandler.errorWhen((!ExternalType(type).isValid()), node,
            "Invalid external type ", uint32_t(uint8_t(type)));
}

void CheckContext::checkEventType(TreeNode* node, const EventType& type)
{
    errorHandler.errorWhen((!EventType(type).isValid()), node,
            "Invalid external type ", uint32_t(uint8_t(type)));
}

void CheckContext::checkLimits(TreeNode* node, const Limits& limits)
{
    errorHandler.errorWhen((limits.hasMax() && limits.max < limits.min), node,
            "Invalid limits; maximum (", limits.max, ") is less then minimum (", limits.min, ')');
}

void CheckContext::checkMut(TreeNode* node, Mut& mut)
{
    errorHandler.errorWhen((mut != Mut::const_ && mut != Mut::var), node,
            "Invalid mut");
}

};
