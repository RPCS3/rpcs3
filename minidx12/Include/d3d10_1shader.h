//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       D3D10_1Shader.h
//  Content:    D3D10.1 Shader Types and APIs
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3D10_1SHADER_H__
#define __D3D10_1SHADER_H__

#include "d3d10shader.h"

#include <winapifamily.h>



//----------------------------------------------------------------------------
// Shader debugging structures
//----------------------------------------------------------------------------

typedef enum _D3D10_SHADER_DEBUG_REGTYPE
{
    D3D10_SHADER_DEBUG_REG_INPUT,
    D3D10_SHADER_DEBUG_REG_OUTPUT,
    D3D10_SHADER_DEBUG_REG_CBUFFER,
    D3D10_SHADER_DEBUG_REG_TBUFFER,
    D3D10_SHADER_DEBUG_REG_TEMP,
    D3D10_SHADER_DEBUG_REG_TEMPARRAY,
    D3D10_SHADER_DEBUG_REG_TEXTURE,
    D3D10_SHADER_DEBUG_REG_SAMPLER,
    D3D10_SHADER_DEBUG_REG_IMMEDIATECBUFFER,
    D3D10_SHADER_DEBUG_REG_LITERAL,
    D3D10_SHADER_DEBUG_REG_UNUSED,
    D3D11_SHADER_DEBUG_REG_INTERFACE_POINTERS,
    D3D11_SHADER_DEBUG_REG_UAV,
    D3D10_SHADER_DEBUG_REG_FORCE_DWORD = 0x7fffffff,
} D3D10_SHADER_DEBUG_REGTYPE;

typedef enum _D3D10_SHADER_DEBUG_SCOPETYPE
{
    D3D10_SHADER_DEBUG_SCOPE_GLOBAL,
    D3D10_SHADER_DEBUG_SCOPE_BLOCK,
    D3D10_SHADER_DEBUG_SCOPE_FORLOOP,
    D3D10_SHADER_DEBUG_SCOPE_STRUCT,
    D3D10_SHADER_DEBUG_SCOPE_FUNC_PARAMS,
    D3D10_SHADER_DEBUG_SCOPE_STATEBLOCK,
    D3D10_SHADER_DEBUG_SCOPE_NAMESPACE,
    D3D10_SHADER_DEBUG_SCOPE_ANNOTATION,
    D3D10_SHADER_DEBUG_SCOPE_FORCE_DWORD = 0x7fffffff,
} D3D10_SHADER_DEBUG_SCOPETYPE;

typedef enum _D3D10_SHADER_DEBUG_VARTYPE
{
    D3D10_SHADER_DEBUG_VAR_VARIABLE,
    D3D10_SHADER_DEBUG_VAR_FUNCTION,
    D3D10_SHADER_DEBUG_VAR_FORCE_DWORD = 0x7fffffff,
} D3D10_SHADER_DEBUG_VARTYPE;

/////////////////////////////////////////////////////////////////////
// These are the serialized structures that get written to the file
/////////////////////////////////////////////////////////////////////

typedef struct _D3D10_SHADER_DEBUG_TOKEN_INFO
{
    UINT File;    // offset into file list
    UINT Line;    // line #
    UINT Column;  // column #

    UINT TokenLength;
    UINT TokenId; // offset to LPCSTR of length TokenLength in string datastore
} D3D10_SHADER_DEBUG_TOKEN_INFO;

// Variable list
typedef struct _D3D10_SHADER_DEBUG_VAR_INFO
{
    // Index into token list for declaring identifier
    UINT TokenId;
    D3D10_SHADER_VARIABLE_TYPE Type;
    // register and component for this variable, only valid/necessary for arrays
    UINT Register;
    UINT Component;
    // gives the original variable that declared this variable
    UINT ScopeVar;
    // this variable's offset in its ScopeVar
    UINT ScopeVarOffset;
} D3D10_SHADER_DEBUG_VAR_INFO;

typedef struct _D3D10_SHADER_DEBUG_INPUT_INFO
{
    // index into array of variables of variable to initialize
    UINT Var;
    // input, cbuffer, tbuffer
    D3D10_SHADER_DEBUG_REGTYPE InitialRegisterSet;
    // set to cbuffer or tbuffer slot, geometry shader input primitive #,
    // identifying register for indexable temp, or -1
    UINT InitialBank;
    // -1 if temp, otherwise gives register in register set
    UINT InitialRegister;
    // -1 if temp, otherwise gives component
    UINT InitialComponent;
    // initial value if literal
    UINT InitialValue;
} D3D10_SHADER_DEBUG_INPUT_INFO;

typedef struct _D3D10_SHADER_DEBUG_SCOPEVAR_INFO
{
    // Index into variable token
    UINT TokenId;

    D3D10_SHADER_DEBUG_VARTYPE VarType; // variable or function (different namespaces)
    D3D10_SHADER_VARIABLE_CLASS Class;
    UINT Rows;          // number of rows (matrices)
    UINT Columns;       // number of columns (vectors and matrices)

    // In an array of structures, one struct member scope is provided, and
    // you'll have to add the array stride times the index to the variable
    // index you find, then find that variable in this structure's list of
    // variables.

    // gives a scope to look up struct members. -1 if not a struct
    UINT StructMemberScope;

    // number of array indices
    UINT uArrayIndices;    // a[3][2][1] has 3 indices
    // maximum array index for each index
    // offset to UINT[uArrayIndices] in UINT datastore
    UINT ArrayElements; // a[3][2][1] has {3, 2, 1}
    // how many variables each array index moves
    // offset to UINT[uArrayIndices] in UINT datastore
    UINT ArrayStrides;  // a[3][2][1] has {2, 1, 1}

    UINT uVariables;
    // index of the first variable, later variables are offsets from this one
    UINT uFirstVariable;
} D3D10_SHADER_DEBUG_SCOPEVAR_INFO;

// scope data, this maps variable names to debug variables (useful for the watch window)
typedef struct _D3D10_SHADER_DEBUG_SCOPE_INFO
{
    D3D10_SHADER_DEBUG_SCOPETYPE ScopeType;
    UINT Name;         // offset to name of scope in strings list
    UINT uNameLen;     // length of name string
    UINT uVariables;
    UINT VariableData; // Offset to UINT[uVariables] indexing the Scope Variable list
} D3D10_SHADER_DEBUG_SCOPE_INFO;

// instruction outputs
typedef struct _D3D10_SHADER_DEBUG_OUTPUTVAR
{
    // index variable being written to, if -1 it's not going to a variable
    UINT Var;
    // range data that the compiler expects to be true
    UINT  uValueMin, uValueMax;
    INT   iValueMin, iValueMax;
    FLOAT fValueMin, fValueMax;

    BOOL  bNaNPossible, bInfPossible;
} D3D10_SHADER_DEBUG_OUTPUTVAR;

typedef struct _D3D10_SHADER_DEBUG_OUTPUTREG_INFO
{
    // Only temp, indexable temp, and output are valid here
    D3D10_SHADER_DEBUG_REGTYPE OutputRegisterSet;
    // -1 means no output
    UINT OutputReg;
    // if a temp array, identifier for which one
    UINT TempArrayReg;
    // -1 means masked out
    UINT OutputComponents[4];
    D3D10_SHADER_DEBUG_OUTPUTVAR OutputVars[4];
    // when indexing the output, get the value of this register, then add
    // that to uOutputReg. If uIndexReg is -1, then there is no index.
    // find the variable whose register is the sum (by looking in the ScopeVar)
    // and component matches, then set it. This should only happen for indexable
    // temps and outputs.
    UINT IndexReg;
    UINT IndexComp;
} D3D10_SHADER_DEBUG_OUTPUTREG_INFO;

// per instruction data
typedef struct _D3D10_SHADER_DEBUG_INST_INFO
{
    UINT Id; // Which instruction this is in the bytecode
    UINT Opcode; // instruction type

    // 0, 1, or 2
    UINT uOutputs;

    // up to two outputs per instruction
    D3D10_SHADER_DEBUG_OUTPUTREG_INFO pOutputs[2];
    
    // index into the list of tokens for this instruction's token
    UINT TokenId;

    // how many function calls deep this instruction is
    UINT NestingLevel;

    // list of scopes from outer-most to inner-most
    // Number of scopes
    UINT Scopes;
    UINT ScopeInfo; // Offset to UINT[uScopes] specifying indices of the ScopeInfo Array

    // list of variables accessed by this instruction
    // Number of variables
    UINT AccessedVars;
    UINT AccessedVarsInfo; // Offset to UINT[AccessedVars] specifying indices of the ScopeVariableInfo Array
} D3D10_SHADER_DEBUG_INST_INFO;

typedef struct _D3D10_SHADER_DEBUG_FILE_INFO
{
    UINT FileName;    // Offset to LPCSTR for file name
    UINT FileNameLen; // Length of file name
    UINT FileData;    // Offset to LPCSTR of length FileLen
    UINT FileLen;     // Length of file
} D3D10_SHADER_DEBUG_FILE_INFO;

typedef struct _D3D10_SHADER_DEBUG_INFO
{
    UINT Size;                             // sizeof(D3D10_SHADER_DEBUG_INFO)
    UINT Creator;                          // Offset to LPCSTR for compiler version
    UINT EntrypointName;                   // Offset to LPCSTR for Entry point name
    UINT ShaderTarget;                     // Offset to LPCSTR for shader target
    UINT CompileFlags;                     // flags used to compile
    UINT Files;                            // number of included files
    UINT FileInfo;                         // Offset to D3D10_SHADER_DEBUG_FILE_INFO[Files]
    UINT Instructions;                     // number of instructions
    UINT InstructionInfo;                  // Offset to D3D10_SHADER_DEBUG_INST_INFO[Instructions]
    UINT Variables;                        // number of variables
    UINT VariableInfo;                     // Offset to D3D10_SHADER_DEBUG_VAR_INFO[Variables]
    UINT InputVariables;                   // number of variables to initialize before running
    UINT InputVariableInfo;                // Offset to D3D10_SHADER_DEBUG_INPUT_INFO[InputVariables]
    UINT Tokens;                           // number of tokens to initialize
    UINT TokenInfo;                        // Offset to D3D10_SHADER_DEBUG_TOKEN_INFO[Tokens]
    UINT Scopes;                           // number of scopes
    UINT ScopeInfo;                        // Offset to D3D10_SHADER_DEBUG_SCOPE_INFO[Scopes]
    UINT ScopeVariables;                   // number of variables declared
    UINT ScopeVariableInfo;                // Offset to D3D10_SHADER_DEBUG_SCOPEVAR_INFO[Scopes]
    UINT UintOffset;                       // Offset to the UINT datastore, all UINT offsets are from this offset
    UINT StringOffset;                     // Offset to the string datastore, all string offsets are from this offset
} D3D10_SHADER_DEBUG_INFO;

//----------------------------------------------------------------------------
// ID3D10ShaderReflection1:
//----------------------------------------------------------------------------

//
// Interface definitions
//


typedef interface ID3D10ShaderReflection1 ID3D10ShaderReflection1;
typedef interface ID3D10ShaderReflection1 *LPD3D10SHADERREFLECTION1;

// {C3457783-A846-47CE-9520-CEA6F66E7447}
DEFINE_GUID(IID_ID3D10ShaderReflection1, 
0xc3457783, 0xa846, 0x47ce, 0x95, 0x20, 0xce, 0xa6, 0xf6, 0x6e, 0x74, 0x47);

#undef INTERFACE
#define INTERFACE ID3D10ShaderReflection1

DECLARE_INTERFACE_(ID3D10ShaderReflection1, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(GetDesc)(THIS_ _Out_ D3D10_SHADER_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3D10ShaderReflectionConstantBuffer*, GetConstantBufferByIndex)(THIS_ UINT Index) PURE;
    STDMETHOD_(ID3D10ShaderReflectionConstantBuffer*, GetConstantBufferByName)(THIS_ LPCSTR Name) PURE;
    
    STDMETHOD(GetResourceBindingDesc)(THIS_ UINT ResourceIndex, _Out_ D3D10_SHADER_INPUT_BIND_DESC *pDesc) PURE;
    
    STDMETHOD(GetInputParameterDesc)(THIS_ UINT ParameterIndex, _Out_ D3D10_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
    STDMETHOD(GetOutputParameterDesc)(THIS_ UINT ParameterIndex, _Out_ D3D10_SIGNATURE_PARAMETER_DESC *pDesc) PURE;

    STDMETHOD_(ID3D10ShaderReflectionVariable*, GetVariableByName)(THIS_ LPCSTR Name) PURE;

    STDMETHOD(GetResourceBindingDescByName)(THIS_ LPCSTR Name, _Out_ D3D10_SHADER_INPUT_BIND_DESC *pDesc) PURE;

    STDMETHOD(GetMovInstructionCount)(THIS_ _Out_ UINT* pCount) PURE;
    STDMETHOD(GetMovcInstructionCount)(THIS_ _Out_ UINT* pCount) PURE;
    STDMETHOD(GetConversionInstructionCount)(THIS_ _Out_ UINT* pCount) PURE;
    STDMETHOD(GetBitwiseInstructionCount)(THIS_ _Out_ UINT* pCount) PURE;
    
    STDMETHOD(GetGSInputPrimitive)(THIS_ _Out_ D3D10_PRIMITIVE* pPrim) PURE;
    STDMETHOD(IsLevel9Shader)(THIS_ _Out_ BOOL* pbLevel9Shader) PURE;
    STDMETHOD(IsSampleFrequencyShader)(THIS_ _Out_ BOOL* pbSampleFrequency) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// APIs //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef __cplusplus
}
#endif //__cplusplus
    
#endif //__D3D10_1SHADER_H__

