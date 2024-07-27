/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WASMModuleParser.h"

#if ENABLE(WEBASSEMBLY)

#include "JSCInlines.h"
#include "JSWASMModule.h"
#include "StrongInlines.h"
#include "WASMMagicNumber.h"
#include <wtf/MathExtras.h>

#define FAIL_WITH_MESSAGE(errorMessage) do { m_errorMessage = errorMessage; return; } while (0)
#define READ_UINT32_OR_FAIL(result, errorMessage) do { if (!m_reader.readUInt32(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_FLOAT_OR_FAIL(result, errorMessage) do { if (!m_reader.readFloat(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_DOUBLE_OR_FAIL(result, errorMessage) do { if (!m_reader.readDouble(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_COMPACT_UINT32_OR_FAIL(result, errorMessage) do { if (!m_reader.readCompactUInt32(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_STRING_OR_FAIL(result, errorMessage) do { if (!m_reader.readString(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_TYPE_OR_FAIL(result, errorMessage) do { if (!m_reader.readType(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_EXPRESSION_TYPE_OR_FAIL(result, errorMessage) do { if (!m_reader.readExpressionType(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define READ_EXPORT_FORMAT_OR_FAIL(result, errorMessage) do { if (!m_reader.readExportFormat(result)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define FAIL_IF_FALSE(condition, errorMessage) do { if (!(condition)) FAIL_WITH_MESSAGE(errorMessage); } while (0)
#define PROPAGATE_ERROR() do { if (!m_errorMessage.isNull()) return; } while (0)

namespace JSC {

WASMModuleParser::WASMModuleParser(const SourceCode& source)
    : m_reader(static_cast<WebAssemblySourceProvider*>(source.provider())->data())
{
}

JSWASMModule* WASMModuleParser::parse(VM& vm, JSGlobalObject* globalObject, String& errorMessage)
{
    m_module.set(vm, JSWASMModule::create(vm, globalObject->wasmModuleStructure()));
    parseModule();
    if (!m_errorMessage.isNull()) {
        errorMessage = m_errorMessage;
        return nullptr;
    }
    return m_module.get();
}

void WASMModuleParser::parseModule()
{
    uint32_t magicNumber;
    READ_UINT32_OR_FAIL(magicNumber, "Cannot read the magic number.");
    FAIL_IF_FALSE(magicNumber == wasmMagicNumber, "The magic number is incorrect.");

    uint32_t outputSizeInASMJS;
    READ_UINT32_OR_FAIL(outputSizeInASMJS, "Cannot read the output size in asm.js format.");

    parseConstantPoolSection();
    PROPAGATE_ERROR();
    parseSignatureSection();
    PROPAGATE_ERROR();
    parseFunctionImportSection();
    PROPAGATE_ERROR();
    parseGlobalSection();
    PROPAGATE_ERROR();
    parseFunctionDeclarationSection();
    PROPAGATE_ERROR();
    parseFunctionPointerTableSection();
    PROPAGATE_ERROR();
    parseFunctionDefinitionSection();
    PROPAGATE_ERROR();
    parseExportSection();
}

void WASMModuleParser::parseConstantPoolSection()
{
    uint32_t numberOfI32Constants;
    uint32_t numberOfF32Constants;
    uint32_t numberOfF64Constants;
    READ_COMPACT_UINT32_OR_FAIL(numberOfI32Constants, "Cannot read the number of int32 constants.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfF32Constants, "Cannot read the number of float32 constants.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfF64Constants, "Cannot read the number of float64 constants.");
    m_module->i32Constants().reserveInitialCapacity(numberOfI32Constants);
    m_module->f32Constants().reserveInitialCapacity(numberOfF32Constants);
    m_module->f64Constants().reserveInitialCapacity(numberOfF64Constants);

    for (uint32_t i = 0; i < numberOfI32Constants; ++i) {
        uint32_t constant;
        READ_COMPACT_UINT32_OR_FAIL(constant, "Cannot read an int32 constant.");
        m_module->i32Constants().uncheckedAppend(constant);
    }
    for (uint32_t i = 0; i < numberOfF32Constants; ++i) {
        float constant;
        READ_FLOAT_OR_FAIL(constant, "Cannot read a float32 constant.");
        m_module->f32Constants().uncheckedAppend(constant);
    }
    for (uint32_t i = 0; i < numberOfF64Constants; ++i) {
        double constant;
        READ_DOUBLE_OR_FAIL(constant, "Cannot read a float64 constant.");
        m_module->f64Constants().uncheckedAppend(constant);
    }
}

void WASMModuleParser::parseSignatureSection()
{
    uint32_t numberOfSignatures;
    READ_COMPACT_UINT32_OR_FAIL(numberOfSignatures, "Cannot read the number of signatures.");
    m_module->signatures().reserveInitialCapacity(numberOfSignatures);
    for (uint32_t signatureIndex = 0; signatureIndex < numberOfSignatures; ++signatureIndex) {
        WASMSignature signature;
        READ_EXPRESSION_TYPE_OR_FAIL(signature.returnType, "Cannot read the return type.");
        uint32_t argumentCount;
        READ_COMPACT_UINT32_OR_FAIL(argumentCount, "Cannot read the number of arguments.");
        signature.arguments.reserveInitialCapacity(argumentCount);
        for (uint32_t argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
            WASMType type;
            READ_TYPE_OR_FAIL(type, "Cannot read the type of an argument.");
            signature.arguments.uncheckedAppend(type);
        }
        m_module->signatures().uncheckedAppend(signature);
    }
}

void WASMModuleParser::parseFunctionImportSection()
{
    uint32_t numberOfFunctionImports;
    uint32_t numberOfFunctionImportSignatures;
    READ_COMPACT_UINT32_OR_FAIL(numberOfFunctionImports, "Cannot read the number of function imports.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfFunctionImportSignatures, "Cannot read the number of function import signatures.");
    m_module->functionImports().reserveInitialCapacity(numberOfFunctionImports);
    m_module->functionImportSignatures().reserveInitialCapacity(numberOfFunctionImportSignatures);

    for (uint32_t functionImportIndex = 0; functionImportIndex < numberOfFunctionImports; ++functionImportIndex) {
        WASMFunctionImport functionImport;
        READ_STRING_OR_FAIL(functionImport.functionName, "Cannot read the function import name.");
        m_module->functionImports().uncheckedAppend(functionImport);

        uint32_t numberOfSignatures;
        READ_COMPACT_UINT32_OR_FAIL(numberOfSignatures, "Cannot read the number of signatures.");
        FAIL_IF_FALSE(numberOfSignatures <= numberOfFunctionImportSignatures - m_module->functionImportSignatures().size(), "The number of signatures is incorrect.");

        for (uint32_t i = 0; i < numberOfSignatures; ++i) {
            WASMFunctionImportSignature functionImportSignature;
            READ_COMPACT_UINT32_OR_FAIL(functionImportSignature.signatureIndex, "Cannot read the signature index.");
            FAIL_IF_FALSE(functionImportSignature.signatureIndex < m_module->signatures().size(), "The signature index is incorrect.");
            functionImportSignature.functionImportIndex = functionImportIndex;
            m_module->functionImportSignatures().uncheckedAppend(functionImportSignature);
        }
    }
    FAIL_IF_FALSE(m_module->functionImportSignatures().size() == numberOfFunctionImportSignatures, "The number of function import signatures is incorrect.");
}

void WASMModuleParser::parseGlobalSection()
{
    uint32_t numberOfInternalI32GlobalVariables;
    uint32_t numberOfInternalF32GlobalVariables;
    uint32_t numberOfInternalF64GlobalVariables;
    uint32_t numberOfImportedI32GlobalVariables;
    uint32_t numberOfImportedF32GlobalVariables;
    uint32_t numberOfImportedF64GlobalVariables;
    READ_COMPACT_UINT32_OR_FAIL(numberOfInternalI32GlobalVariables, "Cannot read the number of internal int32 global variables.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfInternalF32GlobalVariables, "Cannot read the number of internal float32 global variables.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfInternalF64GlobalVariables, "Cannot read the number of internal float64 global variables.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfImportedI32GlobalVariables, "Cannot read the number of imported int32 global variables.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfImportedF32GlobalVariables, "Cannot read the number of imported float32 global variables.");
    READ_COMPACT_UINT32_OR_FAIL(numberOfImportedF64GlobalVariables, "Cannot read the number of imported float64 global variables.");
    uint32_t numberOfGlobalVariables = numberOfInternalI32GlobalVariables + numberOfInternalF32GlobalVariables + numberOfInternalF64GlobalVariables +
        numberOfImportedI32GlobalVariables + numberOfImportedF32GlobalVariables + numberOfImportedF64GlobalVariables;

    Vector<WASMType>& globalVariableTypes = m_module->globalVariableTypes();
    globalVariableTypes.reserveInitialCapacity(numberOfGlobalVariables);
    for (uint32_t i = 0; i < numberOfInternalI32GlobalVariables; ++i)
        globalVariableTypes.uncheckedAppend(WASMType::I32);
    for (uint32_t i = 0; i < numberOfInternalF32GlobalVariables; ++i)
        globalVariableTypes.uncheckedAppend(WASMType::F32);
    for (uint32_t i = 0; i < numberOfInternalF64GlobalVariables; ++i)
        globalVariableTypes.uncheckedAppend(WASMType::F64);
    for (uint32_t i = 0; i < numberOfImportedI32GlobalVariables; ++i) {
        String importName;
        READ_STRING_OR_FAIL(importName, "Cannot read the import name of an int32 global variable.");
        globalVariableTypes.uncheckedAppend(WASMType::I32);
    }
    for (uint32_t i = 0; i < numberOfImportedF32GlobalVariables; ++i) {
        String importName;
        READ_STRING_OR_FAIL(importName, "Cannot read the import name of a float32 global variable.");
        globalVariableTypes.uncheckedAppend(WASMType::F32);
    }
    for (uint32_t i = 0; i < numberOfImportedF64GlobalVariables; ++i) {
        String importName;
        READ_STRING_OR_FAIL(importName, "Cannot read the import name of a float64 global variable.");
        globalVariableTypes.uncheckedAppend(WASMType::F64);
    }
}

void WASMModuleParser::parseFunctionDeclarationSection()
{
    uint32_t numberOfFunctionDeclarations;
    READ_COMPACT_UINT32_OR_FAIL(numberOfFunctionDeclarations, "Cannot read the number of function declarations.");
    m_module->functionDeclarations().reserveInitialCapacity(numberOfFunctionDeclarations);
    for (uint32_t i = 0; i < numberOfFunctionDeclarations; ++i) {
        WASMFunctionDeclaration functionDeclaration;
        READ_COMPACT_UINT32_OR_FAIL(functionDeclaration.signatureIndex, "Cannot read the signature index.");
        FAIL_IF_FALSE(functionDeclaration.signatureIndex < m_module->signatures().size(), "The signature index is incorrect.");
        m_module->functionDeclarations().uncheckedAppend(functionDeclaration);
    }
}

void WASMModuleParser::parseFunctionPointerTableSection()
{
    uint32_t numberOfFunctionPointerTables;
    READ_COMPACT_UINT32_OR_FAIL(numberOfFunctionPointerTables, "Cannot read the number of function pointer tables.");
    m_module->functionPointerTables().reserveInitialCapacity(numberOfFunctionPointerTables);
    for (uint32_t i = 0; i < numberOfFunctionPointerTables; ++i) {
        WASMFunctionPointerTable functionPointerTable;
        READ_COMPACT_UINT32_OR_FAIL(functionPointerTable.signatureIndex, "Cannot read the signature index.");
        FAIL_IF_FALSE(functionPointerTable.signatureIndex < m_module->signatures().size(), "The signature index is incorrect.");
        uint32_t numberOfElements;
        READ_COMPACT_UINT32_OR_FAIL(numberOfElements, "Cannot read the number of elements of a function pointer table.");
        FAIL_IF_FALSE(hasOneBitSet(numberOfElements), "The number of elements must be a power of two.");
        functionPointerTable.elements.reserveInitialCapacity(numberOfElements);
        for (uint32_t j = 0; j < numberOfElements; ++j) {
            uint32_t element;
            READ_COMPACT_UINT32_OR_FAIL(element, "Cannot read an element of a function pointer table.");
            functionPointerTable.elements.uncheckedAppend(element);
        }
        m_module->functionPointerTables().uncheckedAppend(functionPointerTable);
    }
}

void WASMModuleParser::parseFunctionDefinitionSection()
{
    for (size_t i = 0; i < m_module->functionDeclarations().size(); ++i) {
        parseFunctionDefinition();
        PROPAGATE_ERROR();
    }
}

void WASMModuleParser::parseFunctionDefinition()
{
    // FIXME: Support any functions. https://bugs.webkit.org/show_bug.cgi?id=147738
    // Currently, we only support functions that have "return 0;" as their only statement.
    // These functions consist of exactly 4 bytes, i.e.
    // 1. The number of local variables (0) [0x80]
    // 2. The number of statements (1) [0x01]
    // 3. The return statement [0x0f]
    // 4. The immediate expression (0) [0xa0]
    uint32_t functionDefinitionBytes;
    READ_UINT32_OR_FAIL(functionDefinitionBytes, "Cannot read the function definition.");
    FAIL_IF_FALSE(functionDefinitionBytes == 0xa00f0180, "Only functions that have \"return 0;\" "
        "as their only statement are supported at the moment.");
}

void WASMModuleParser::parseExportSection()
{
    WASMExportFormat exportFormat;
    READ_EXPORT_FORMAT_OR_FAIL(exportFormat, "Cannot read the export format.");
    switch (exportFormat) {
    case WASMExportFormat::Default: {
        uint32_t functionIndex;
        READ_COMPACT_UINT32_OR_FAIL(functionIndex, "Cannot read the function index.");
        FAIL_IF_FALSE(functionIndex < m_module->functionDeclarations().size(), "The function index is incorrect.");
        // FIXME: Export the function.
        break;
    }
    case WASMExportFormat::Record: {
        uint32_t numberOfExports;
        READ_COMPACT_UINT32_OR_FAIL(numberOfExports, "Cannot read the number of exports.");
        for (uint32_t exportIndex = 0; exportIndex < numberOfExports; ++exportIndex) {
            String exportName;
            READ_STRING_OR_FAIL(exportName, "Cannot read the function export name.");
            // FIXME: Check that exportName is legal.
            uint32_t functionIndex;
            READ_COMPACT_UINT32_OR_FAIL(functionIndex, "Cannot read the function index.");
            FAIL_IF_FALSE(functionIndex < m_module->functionDeclarations().size(), "The function index is incorrect.");
            // FIXME: Export the function.
        }
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
}

JSWASMModule* parseWebAssembly(ExecState* exec, const SourceCode& source, String& errorMessage)
{
    WASMModuleParser WASMModuleParser(source);
    return WASMModuleParser.parse(exec->vm(), exec->lexicalGlobalObject(), errorMessage);
}

} // namespace JSC

#endif // ENABLE(WEBASSEMBLY)
