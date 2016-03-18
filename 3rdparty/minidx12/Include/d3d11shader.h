//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       D3D11Shader.h
//  Content:    D3D11 Shader Types and APIs
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3D11SHADER_H__
#define __D3D11SHADER_H__

#include "d3dcommon.h"


typedef enum D3D11_SHADER_VERSION_TYPE
{
    D3D11_SHVER_PIXEL_SHADER    = 0,
    D3D11_SHVER_VERTEX_SHADER   = 1,
    D3D11_SHVER_GEOMETRY_SHADER = 2,
    
    // D3D11 Shaders
    D3D11_SHVER_HULL_SHADER     = 3,
    D3D11_SHVER_DOMAIN_SHADER   = 4,
    D3D11_SHVER_COMPUTE_SHADER  = 5,

    D3D11_SHVER_RESERVED0       = 0xFFF0,
} D3D11_SHADER_VERSION_TYPE;

#define D3D11_SHVER_GET_TYPE(_Version) \
    (((_Version) >> 16) & 0xffff)
#define D3D11_SHVER_GET_MAJOR(_Version) \
    (((_Version) >> 4) & 0xf)
#define D3D11_SHVER_GET_MINOR(_Version) \
    (((_Version) >> 0) & 0xf)

// Slot ID for library function return
#define D3D_RETURN_PARAMETER_INDEX (-1)

typedef D3D_RESOURCE_RETURN_TYPE D3D11_RESOURCE_RETURN_TYPE;

typedef D3D_CBUFFER_TYPE D3D11_CBUFFER_TYPE;


typedef struct _D3D11_SIGNATURE_PARAMETER_DESC
{
    LPCSTR                      SemanticName;   // Name of the semantic
    UINT                        SemanticIndex;  // Index of the semantic
    UINT                        Register;       // Number of member variables
    D3D_NAME                    SystemValueType;// A predefined system value, or D3D_NAME_UNDEFINED if not applicable
    D3D_REGISTER_COMPONENT_TYPE ComponentType;  // Scalar type (e.g. uint, float, etc.)
    BYTE                        Mask;           // Mask to indicate which components of the register
                                                // are used (combination of D3D10_COMPONENT_MASK values)
    BYTE                        ReadWriteMask;  // Mask to indicate whether a given component is 
                                                // never written (if this is an output signature) or
                                                // always read (if this is an input signature).
                                                // (combination of D3D_MASK_* values)
    UINT                        Stream;         // Stream index
    D3D_MIN_PRECISION           MinPrecision;   // Minimum desired interpolation precision
} D3D11_SIGNATURE_PARAMETER_DESC;

typedef struct _D3D11_SHADER_BUFFER_DESC
{
    LPCSTR                  Name;           // Name of the constant buffer
    D3D_CBUFFER_TYPE        Type;           // Indicates type of buffer content
    UINT                    Variables;      // Number of member variables
    UINT                    Size;           // Size of CB (in bytes)
    UINT                    uFlags;         // Buffer description flags
} D3D11_SHADER_BUFFER_DESC;

typedef struct _D3D11_SHADER_VARIABLE_DESC
{
    LPCSTR                  Name;           // Name of the variable
    UINT                    StartOffset;    // Offset in constant buffer's backing store
    UINT                    Size;           // Size of variable (in bytes)
    UINT                    uFlags;         // Variable flags
    LPVOID                  DefaultValue;   // Raw pointer to default value
    UINT                    StartTexture;   // First texture index (or -1 if no textures used)
    UINT                    TextureSize;    // Number of texture slots possibly used.
    UINT                    StartSampler;   // First sampler index (or -1 if no textures used)
    UINT                    SamplerSize;    // Number of sampler slots possibly used.
} D3D11_SHADER_VARIABLE_DESC;

typedef struct _D3D11_SHADER_TYPE_DESC
{
    D3D_SHADER_VARIABLE_CLASS   Class;          // Variable class (e.g. object, matrix, etc.)
    D3D_SHADER_VARIABLE_TYPE    Type;           // Variable type (e.g. float, sampler, etc.)
    UINT                        Rows;           // Number of rows (for matrices, 1 for other numeric, 0 if not applicable)
    UINT                        Columns;        // Number of columns (for vectors & matrices, 1 for other numeric, 0 if not applicable)
    UINT                        Elements;       // Number of elements (0 if not an array)
    UINT                        Members;        // Number of members (0 if not a structure)
    UINT                        Offset;         // Offset from the start of structure (0 if not a structure member)
    LPCSTR                      Name;           // Name of type, can be NULL
} D3D11_SHADER_TYPE_DESC;

typedef D3D_TESSELLATOR_DOMAIN D3D11_TESSELLATOR_DOMAIN;

typedef D3D_TESSELLATOR_PARTITIONING D3D11_TESSELLATOR_PARTITIONING;

typedef D3D_TESSELLATOR_OUTPUT_PRIMITIVE D3D11_TESSELLATOR_OUTPUT_PRIMITIVE;

typedef struct _D3D11_SHADER_DESC
{
    UINT                    Version;                     // Shader version
    LPCSTR                  Creator;                     // Creator string
    UINT                    Flags;                       // Shader compilation/parse flags
    
    UINT                    ConstantBuffers;             // Number of constant buffers
    UINT                    BoundResources;              // Number of bound resources
    UINT                    InputParameters;             // Number of parameters in the input signature
    UINT                    OutputParameters;            // Number of parameters in the output signature

    UINT                    InstructionCount;            // Number of emitted instructions
    UINT                    TempRegisterCount;           // Number of temporary registers used 
    UINT                    TempArrayCount;              // Number of temporary arrays used
    UINT                    DefCount;                    // Number of constant defines 
    UINT                    DclCount;                    // Number of declarations (input + output)
    UINT                    TextureNormalInstructions;   // Number of non-categorized texture instructions
    UINT                    TextureLoadInstructions;     // Number of texture load instructions
    UINT                    TextureCompInstructions;     // Number of texture comparison instructions
    UINT                    TextureBiasInstructions;     // Number of texture bias instructions
    UINT                    TextureGradientInstructions; // Number of texture gradient instructions
    UINT                    FloatInstructionCount;       // Number of floating point arithmetic instructions used
    UINT                    IntInstructionCount;         // Number of signed integer arithmetic instructions used
    UINT                    UintInstructionCount;        // Number of unsigned integer arithmetic instructions used
    UINT                    StaticFlowControlCount;      // Number of static flow control instructions used
    UINT                    DynamicFlowControlCount;     // Number of dynamic flow control instructions used
    UINT                    MacroInstructionCount;       // Number of macro instructions used
    UINT                    ArrayInstructionCount;       // Number of array instructions used
    UINT                    CutInstructionCount;         // Number of cut instructions used
    UINT                    EmitInstructionCount;        // Number of emit instructions used
    D3D_PRIMITIVE_TOPOLOGY  GSOutputTopology;            // Geometry shader output topology
    UINT                    GSMaxOutputVertexCount;      // Geometry shader maximum output vertex count
    D3D_PRIMITIVE           InputPrimitive;              // GS/HS input primitive
    UINT                    PatchConstantParameters;     // Number of parameters in the patch constant signature
    UINT                    cGSInstanceCount;            // Number of Geometry shader instances
    UINT                    cControlPoints;              // Number of control points in the HS->DS stage
    D3D_TESSELLATOR_OUTPUT_PRIMITIVE HSOutputPrimitive;  // Primitive output by the tessellator
    D3D_TESSELLATOR_PARTITIONING HSPartitioning;         // Partitioning mode of the tessellator
    D3D_TESSELLATOR_DOMAIN  TessellatorDomain;           // Domain of the tessellator (quad, tri, isoline)
    // instruction counts
    UINT cBarrierInstructions;                           // Number of barrier instructions in a compute shader
    UINT cInterlockedInstructions;                       // Number of interlocked instructions
    UINT cTextureStoreInstructions;                      // Number of texture writes
} D3D11_SHADER_DESC;

typedef struct _D3D11_SHADER_INPUT_BIND_DESC
{
    LPCSTR                      Name;           // Name of the resource
    D3D_SHADER_INPUT_TYPE       Type;           // Type of resource (e.g. texture, cbuffer, etc.)
    UINT                        BindPoint;      // Starting bind point
    UINT                        BindCount;      // Number of contiguous bind points (for arrays)
    
    UINT                        uFlags;         // Input binding flags
    D3D_RESOURCE_RETURN_TYPE    ReturnType;     // Return type (if texture)
    D3D_SRV_DIMENSION           Dimension;      // Dimension (if texture)
    UINT                        NumSamples;     // Number of samples (0 if not MS texture)
} D3D11_SHADER_INPUT_BIND_DESC;

#define D3D_SHADER_REQUIRES_DOUBLES                         0x00000001
#define D3D_SHADER_REQUIRES_EARLY_DEPTH_STENCIL             0x00000002
#define D3D_SHADER_REQUIRES_UAVS_AT_EVERY_STAGE             0x00000004
#define D3D_SHADER_REQUIRES_64_UAVS                         0x00000008
#define D3D_SHADER_REQUIRES_MINIMUM_PRECISION               0x00000010
#define D3D_SHADER_REQUIRES_11_1_DOUBLE_EXTENSIONS          0x00000020
#define D3D_SHADER_REQUIRES_11_1_SHADER_EXTENSIONS          0x00000040
#define D3D_SHADER_REQUIRES_LEVEL_9_COMPARISON_FILTERING    0x00000080
#define D3D_SHADER_REQUIRES_TILED_RESOURCES                 0x00000100


typedef struct _D3D11_LIBRARY_DESC
{
    LPCSTR    Creator;           // The name of the originator of the library.
    UINT      Flags;             // Compilation flags.
    UINT      FunctionCount;     // Number of functions exported from the library.
} D3D11_LIBRARY_DESC;

typedef struct _D3D11_FUNCTION_DESC
{
    UINT                    Version;                     // Shader version
    LPCSTR                  Creator;                     // Creator string
    UINT                    Flags;                       // Shader compilation/parse flags
    
    UINT                    ConstantBuffers;             // Number of constant buffers
    UINT                    BoundResources;              // Number of bound resources

    UINT                    InstructionCount;            // Number of emitted instructions
    UINT                    TempRegisterCount;           // Number of temporary registers used 
    UINT                    TempArrayCount;              // Number of temporary arrays used
    UINT                    DefCount;                    // Number of constant defines 
    UINT                    DclCount;                    // Number of declarations (input + output)
    UINT                    TextureNormalInstructions;   // Number of non-categorized texture instructions
    UINT                    TextureLoadInstructions;     // Number of texture load instructions
    UINT                    TextureCompInstructions;     // Number of texture comparison instructions
    UINT                    TextureBiasInstructions;     // Number of texture bias instructions
    UINT                    TextureGradientInstructions; // Number of texture gradient instructions
    UINT                    FloatInstructionCount;       // Number of floating point arithmetic instructions used
    UINT                    IntInstructionCount;         // Number of signed integer arithmetic instructions used
    UINT                    UintInstructionCount;        // Number of unsigned integer arithmetic instructions used
    UINT                    StaticFlowControlCount;      // Number of static flow control instructions used
    UINT                    DynamicFlowControlCount;     // Number of dynamic flow control instructions used
    UINT                    MacroInstructionCount;       // Number of macro instructions used
    UINT                    ArrayInstructionCount;       // Number of array instructions used
    UINT                    MovInstructionCount;         // Number of mov instructions used
    UINT                    MovcInstructionCount;        // Number of movc instructions used
    UINT                    ConversionInstructionCount;  // Number of type conversion instructions used
    UINT                    BitwiseInstructionCount;     // Number of bitwise arithmetic instructions used
    D3D_FEATURE_LEVEL       MinFeatureLevel;             // Min target of the function byte code
    UINT64                  RequiredFeatureFlags;        // Required feature flags

    LPCSTR                  Name;                        // Function name
    INT                     FunctionParameterCount;      // Number of logical parameters in the function signature (not including return)
    BOOL                    HasReturn;                   // TRUE, if function returns a value, false - it is a subroutine
    BOOL                    Has10Level9VertexShader;     // TRUE, if there is a 10L9 VS blob
    BOOL                    Has10Level9PixelShader;      // TRUE, if there is a 10L9 PS blob
} D3D11_FUNCTION_DESC;

typedef struct _D3D11_PARAMETER_DESC
{
    LPCSTR                      Name;               // Parameter name.
    LPCSTR                      SemanticName;       // Parameter semantic name (+index).
    D3D_SHADER_VARIABLE_TYPE    Type;               // Element type.
    D3D_SHADER_VARIABLE_CLASS   Class;              // Scalar/Vector/Matrix.
    UINT                        Rows;               // Rows are for matrix parameters.
    UINT                        Columns;            // Components or Columns in matrix.
    D3D_INTERPOLATION_MODE      InterpolationMode;  // Interpolation mode.
    D3D_PARAMETER_FLAGS         Flags;              // Parameter modifiers.

    UINT                        FirstInRegister;    // The first input register for this parameter.
    UINT                        FirstInComponent;   // The first input register component for this parameter.
    UINT                        FirstOutRegister;   // The first output register for this parameter.
    UINT                        FirstOutComponent;  // The first output register component for this parameter.
} D3D11_PARAMETER_DESC;


//////////////////////////////////////////////////////////////////////////////
// Interfaces ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3D11ShaderReflectionType ID3D11ShaderReflectionType;
typedef interface ID3D11ShaderReflectionType *LPD3D11SHADERREFLECTIONTYPE;

typedef interface ID3D11ShaderReflectionVariable ID3D11ShaderReflectionVariable;
typedef interface ID3D11ShaderReflectionVariable *LPD3D11SHADERREFLECTIONVARIABLE;

typedef interface ID3D11ShaderReflectionConstantBuffer ID3D11ShaderReflectionConstantBuffer;
typedef interface ID3D11ShaderReflectionConstantBuffer *LPD3D11SHADERREFLECTIONCONSTANTBUFFER;

typedef interface ID3D11ShaderReflection ID3D11ShaderReflection;
typedef interface ID3D11ShaderReflection *LPD3D11SHADERREFLECTION;

typedef interface ID3D11LibraryReflection ID3D11LibraryReflection;
typedef interface ID3D11LibraryReflection *LPD3D11LIBRARYREFLECTION;

typedef interface ID3D11FunctionReflection ID3D11FunctionReflection;
typedef interface ID3D11FunctionReflection *LPD3D11FUNCTIONREFLECTION;

typedef interface ID3D11FunctionParameterReflection ID3D11FunctionParameterReflection;
typedef interface ID3D11FunctionParameterReflection *LPD3D11FUNCTIONPARAMETERREFLECTION;

// {6E6FFA6A-9BAE-4613-A51E-91652D508C21}
interface DECLSPEC_UUID("6E6FFA6A-9BAE-4613-A51E-91652D508C21") ID3D11ShaderReflectionType;
DEFINE_GUID(IID_ID3D11ShaderReflectionType, 
0x6e6ffa6a, 0x9bae, 0x4613, 0xa5, 0x1e, 0x91, 0x65, 0x2d, 0x50, 0x8c, 0x21);

#undef INTERFACE
#define INTERFACE ID3D11ShaderReflectionType

DECLARE_INTERFACE(ID3D11ShaderReflectionType)
{
    STDMETHOD(GetDesc)(THIS_ _Out_ D3D11_SHADER_TYPE_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3D11ShaderReflectionType*, GetMemberTypeByIndex)(THIS_ _In_ UINT Index) PURE;
    STDMETHOD_(ID3D11ShaderReflectionType*, GetMemberTypeByName)(THIS_ _In_ LPCSTR Name) PURE;
    STDMETHOD_(LPCSTR, GetMemberTypeName)(THIS_ _In_ UINT Index) PURE;

    STDMETHOD(IsEqual)(THIS_ _In_ ID3D11ShaderReflectionType* pType) PURE;
    STDMETHOD_(ID3D11ShaderReflectionType*, GetSubType)(THIS) PURE;
    STDMETHOD_(ID3D11ShaderReflectionType*, GetBaseClass)(THIS) PURE;
    STDMETHOD_(UINT, GetNumInterfaces)(THIS) PURE;
    STDMETHOD_(ID3D11ShaderReflectionType*, GetInterfaceByIndex)(THIS_ _In_ UINT uIndex) PURE;
    STDMETHOD(IsOfType)(THIS_ _In_ ID3D11ShaderReflectionType* pType) PURE;
    STDMETHOD(ImplementsInterface)(THIS_ _In_ ID3D11ShaderReflectionType* pBase) PURE;
};

// {51F23923-F3E5-4BD1-91CB-606177D8DB4C}
interface DECLSPEC_UUID("51F23923-F3E5-4BD1-91CB-606177D8DB4C") ID3D11ShaderReflectionVariable;
DEFINE_GUID(IID_ID3D11ShaderReflectionVariable, 
0x51f23923, 0xf3e5, 0x4bd1, 0x91, 0xcb, 0x60, 0x61, 0x77, 0xd8, 0xdb, 0x4c);

#undef INTERFACE
#define INTERFACE ID3D11ShaderReflectionVariable

DECLARE_INTERFACE(ID3D11ShaderReflectionVariable)
{
    STDMETHOD(GetDesc)(THIS_ _Out_ D3D11_SHADER_VARIABLE_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3D11ShaderReflectionType*, GetType)(THIS) PURE;
    STDMETHOD_(ID3D11ShaderReflectionConstantBuffer*, GetBuffer)(THIS) PURE;

    STDMETHOD_(UINT, GetInterfaceSlot)(THIS_ _In_ UINT uArrayIndex) PURE;
};

// {EB62D63D-93DD-4318-8AE8-C6F83AD371B8}
interface DECLSPEC_UUID("EB62D63D-93DD-4318-8AE8-C6F83AD371B8") ID3D11ShaderReflectionConstantBuffer;
DEFINE_GUID(IID_ID3D11ShaderReflectionConstantBuffer, 
0xeb62d63d, 0x93dd, 0x4318, 0x8a, 0xe8, 0xc6, 0xf8, 0x3a, 0xd3, 0x71, 0xb8);

#undef INTERFACE
#define INTERFACE ID3D11ShaderReflectionConstantBuffer

DECLARE_INTERFACE(ID3D11ShaderReflectionConstantBuffer)
{
    STDMETHOD(GetDesc)(THIS_ D3D11_SHADER_BUFFER_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3D11ShaderReflectionVariable*, GetVariableByIndex)(THIS_ _In_ UINT Index) PURE;
    STDMETHOD_(ID3D11ShaderReflectionVariable*, GetVariableByName)(THIS_ _In_ LPCSTR Name) PURE;
};

// The ID3D11ShaderReflection IID may change from SDK version to SDK version
// if the reflection API changes.  This prevents new code with the new API
// from working with an old binary.  Recompiling with the new header
// will pick up the new IID.

// 8d536ca1-0cca-4956-a837-786963755584
interface DECLSPEC_UUID("8d536ca1-0cca-4956-a837-786963755584") ID3D11ShaderReflection;
DEFINE_GUID(IID_ID3D11ShaderReflection,
0x8d536ca1, 0x0cca, 0x4956, 0xa8, 0x37, 0x78, 0x69, 0x63, 0x75, 0x55, 0x84);

#undef INTERFACE
#define INTERFACE ID3D11ShaderReflection

DECLARE_INTERFACE_(ID3D11ShaderReflection, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid,
                              _Out_ LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(GetDesc)(THIS_ _Out_ D3D11_SHADER_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3D11ShaderReflectionConstantBuffer*, GetConstantBufferByIndex)(THIS_ _In_ UINT Index) PURE;
    STDMETHOD_(ID3D11ShaderReflectionConstantBuffer*, GetConstantBufferByName)(THIS_ _In_ LPCSTR Name) PURE;
    
    STDMETHOD(GetResourceBindingDesc)(THIS_ _In_ UINT ResourceIndex,
                                      _Out_ D3D11_SHADER_INPUT_BIND_DESC *pDesc) PURE;
    
    STDMETHOD(GetInputParameterDesc)(THIS_ _In_ UINT ParameterIndex,
                                     _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
    STDMETHOD(GetOutputParameterDesc)(THIS_ _In_ UINT ParameterIndex,
                                      _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
    STDMETHOD(GetPatchConstantParameterDesc)(THIS_ _In_ UINT ParameterIndex,
                                             _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;

    STDMETHOD_(ID3D11ShaderReflectionVariable*, GetVariableByName)(THIS_ _In_ LPCSTR Name) PURE;

    STDMETHOD(GetResourceBindingDescByName)(THIS_ _In_ LPCSTR Name,
                                            _Out_ D3D11_SHADER_INPUT_BIND_DESC *pDesc) PURE;

    STDMETHOD_(UINT, GetMovInstructionCount)(THIS) PURE;
    STDMETHOD_(UINT, GetMovcInstructionCount)(THIS) PURE;
    STDMETHOD_(UINT, GetConversionInstructionCount)(THIS) PURE;
    STDMETHOD_(UINT, GetBitwiseInstructionCount)(THIS) PURE;
    
    STDMETHOD_(D3D_PRIMITIVE, GetGSInputPrimitive)(THIS) PURE;
    STDMETHOD_(BOOL, IsSampleFrequencyShader)(THIS) PURE;

    STDMETHOD_(UINT, GetNumInterfaceSlots)(THIS) PURE;
    STDMETHOD(GetMinFeatureLevel)(THIS_ _Out_ enum D3D_FEATURE_LEVEL* pLevel) PURE;

    STDMETHOD_(UINT, GetThreadGroupSize)(THIS_
                                         _Out_opt_ UINT* pSizeX,
                                         _Out_opt_ UINT* pSizeY,
                                         _Out_opt_ UINT* pSizeZ) PURE;

    STDMETHOD_(UINT64, GetRequiresFlags)(THIS) PURE;
};

// {54384F1B-5B3E-4BB7-AE01-60BA3097CBB6}
interface DECLSPEC_UUID("54384F1B-5B3E-4BB7-AE01-60BA3097CBB6") ID3D11LibraryReflection;
DEFINE_GUID(IID_ID3D11LibraryReflection, 
0x54384f1b, 0x5b3e, 0x4bb7, 0xae, 0x1, 0x60, 0xba, 0x30, 0x97, 0xcb, 0xb6);

#undef INTERFACE
#define INTERFACE ID3D11LibraryReflection

DECLARE_INTERFACE_(ID3D11LibraryReflection, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(GetDesc)(THIS_ _Out_ D3D11_LIBRARY_DESC * pDesc) PURE;
    
    STDMETHOD_(ID3D11FunctionReflection *, GetFunctionByIndex)(THIS_ _In_ INT FunctionIndex) PURE;
};

// {207BCECB-D683-4A06-A8A3-9B149B9F73A4}
interface DECLSPEC_UUID("207BCECB-D683-4A06-A8A3-9B149B9F73A4") ID3D11FunctionReflection;
DEFINE_GUID(IID_ID3D11FunctionReflection, 
0x207bcecb, 0xd683, 0x4a06, 0xa8, 0xa3, 0x9b, 0x14, 0x9b, 0x9f, 0x73, 0xa4);

#undef INTERFACE
#define INTERFACE ID3D11FunctionReflection

DECLARE_INTERFACE(ID3D11FunctionReflection)
{
    STDMETHOD(GetDesc)(THIS_ _Out_ D3D11_FUNCTION_DESC * pDesc) PURE;
    
    STDMETHOD_(ID3D11ShaderReflectionConstantBuffer *, GetConstantBufferByIndex)(THIS_ _In_ UINT BufferIndex) PURE;
    STDMETHOD_(ID3D11ShaderReflectionConstantBuffer *, GetConstantBufferByName)(THIS_ _In_ LPCSTR Name) PURE;
    
    STDMETHOD(GetResourceBindingDesc)(THIS_ _In_ UINT ResourceIndex,
                                      _Out_ D3D11_SHADER_INPUT_BIND_DESC * pDesc) PURE;
    
    STDMETHOD_(ID3D11ShaderReflectionVariable *, GetVariableByName)(THIS_ _In_ LPCSTR Name) PURE;

    STDMETHOD(GetResourceBindingDescByName)(THIS_ _In_ LPCSTR Name,
                                            _Out_ D3D11_SHADER_INPUT_BIND_DESC * pDesc) PURE;

    // Use D3D_RETURN_PARAMETER_INDEX to get description of the return value.
    STDMETHOD_(ID3D11FunctionParameterReflection *, GetFunctionParameter)(THIS_ _In_ INT ParameterIndex) PURE;
};

// {42757488-334F-47FE-982E-1A65D08CC462}
interface DECLSPEC_UUID("42757488-334F-47FE-982E-1A65D08CC462") ID3D11FunctionParameterReflection;
DEFINE_GUID(IID_ID3D11FunctionParameterReflection, 
0x42757488, 0x334f, 0x47fe, 0x98, 0x2e, 0x1a, 0x65, 0xd0, 0x8c, 0xc4, 0x62);

#undef INTERFACE
#define INTERFACE ID3D11FunctionParameterReflection

DECLARE_INTERFACE(ID3D11FunctionParameterReflection)
{
    STDMETHOD(GetDesc)(THIS_ _Out_ D3D11_PARAMETER_DESC * pDesc) PURE;
};

// {CAC701EE-80FC-4122-8242-10B39C8CEC34}
interface DECLSPEC_UUID("CAC701EE-80FC-4122-8242-10B39C8CEC34") ID3D11Module;
DEFINE_GUID(IID_ID3D11Module, 
0xcac701ee, 0x80fc, 0x4122, 0x82, 0x42, 0x10, 0xb3, 0x9c, 0x8c, 0xec, 0x34);

#undef INTERFACE
#define INTERFACE ID3D11Module

DECLARE_INTERFACE_(ID3D11Module, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Create an instance of a module for resource re-binding.
    STDMETHOD(CreateInstance)(THIS_ _In_opt_ LPCSTR pNamespace,
                                    _COM_Outptr_ interface ID3D11ModuleInstance ** ppModuleInstance) PURE;
};


// {469E07F7-045A-48D5-AA12-68A478CDF75D}
interface DECLSPEC_UUID("469E07F7-045A-48D5-AA12-68A478CDF75D") ID3D11ModuleInstance;
DEFINE_GUID(IID_ID3D11ModuleInstance, 
0x469e07f7, 0x45a, 0x48d5, 0xaa, 0x12, 0x68, 0xa4, 0x78, 0xcd, 0xf7, 0x5d);

#undef INTERFACE
#define INTERFACE ID3D11ModuleInstance

DECLARE_INTERFACE_(ID3D11ModuleInstance, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // Resource binding API.
    //
    STDMETHOD(BindConstantBuffer)(THIS_ _In_ UINT uSrcSlot, _In_ UINT uDstSlot, _In_ UINT cbDstOffset) PURE;
    STDMETHOD(BindConstantBufferByName)(THIS_ _In_ LPCSTR pName, _In_ UINT uDstSlot, _In_ UINT cbDstOffset) PURE;

    STDMETHOD(BindResource)(THIS_ _In_ UINT uSrcSlot, _In_ UINT uDstSlot, _In_ UINT uCount) PURE;
    STDMETHOD(BindResourceByName)(THIS_ _In_ LPCSTR pName, _In_ UINT uDstSlot, _In_ UINT uCount) PURE;

    STDMETHOD(BindSampler)(THIS_ _In_ UINT uSrcSlot, _In_ UINT uDstSlot, _In_ UINT uCount) PURE;
    STDMETHOD(BindSamplerByName)(THIS_ _In_ LPCSTR pName, _In_ UINT uDstSlot, _In_ UINT uCount) PURE;

    STDMETHOD(BindUnorderedAccessView)(THIS_ _In_ UINT uSrcSlot, _In_ UINT uDstSlot, _In_ UINT uCount) PURE;
    STDMETHOD(BindUnorderedAccessViewByName)(THIS_ _In_ LPCSTR pName, _In_ UINT uDstSlot, _In_ UINT uCount) PURE;

    STDMETHOD(BindResourceAsUnorderedAccessView)(THIS_ _In_ UINT uSrcSrvSlot, _In_ UINT uDstUavSlot, _In_ UINT uCount) PURE;
    STDMETHOD(BindResourceAsUnorderedAccessViewByName)(THIS_ _In_ LPCSTR pSrvName, _In_ UINT uDstUavSlot, _In_ UINT uCount) PURE;
};


// {59A6CD0E-E10D-4C1F-88C0-63ABA1DAF30E}
interface DECLSPEC_UUID("59A6CD0E-E10D-4C1F-88C0-63ABA1DAF30E") ID3D11Linker;
DEFINE_GUID(IID_ID3D11Linker, 
0x59a6cd0e, 0xe10d, 0x4c1f, 0x88, 0xc0, 0x63, 0xab, 0xa1, 0xda, 0xf3, 0xe);

#undef INTERFACE
#define INTERFACE ID3D11Linker

DECLARE_INTERFACE_(ID3D11Linker, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Link the shader and produce a shader blob suitable to D3D runtime.
    STDMETHOD(Link)(THIS_ _In_ interface ID3D11ModuleInstance * pEntry,
                          _In_ LPCSTR pEntryName,
                          _In_ LPCSTR pTargetName,
                          _In_ UINT uFlags,
                          _COM_Outptr_ ID3DBlob ** ppShaderBlob,
                          _Always_(_Outptr_opt_result_maybenull_) ID3DBlob ** ppErrorBuffer) PURE;

    // Add an instance of a library module to be used for linking.
    STDMETHOD(UseLibrary)(THIS_ _In_ interface ID3D11ModuleInstance * pLibraryMI) PURE;

    // Add a clip plane with the plane coefficients taken from a cbuffer entry for 10L9 shaders.
    STDMETHOD(AddClipPlaneFromCBuffer)(THIS_ _In_ UINT uCBufferSlot, _In_ UINT uCBufferEntry) PURE;
};


// {D80DD70C-8D2F-4751-94A1-03C79B3556DB}
interface DECLSPEC_UUID("D80DD70C-8D2F-4751-94A1-03C79B3556DB") ID3D11LinkingNode;
DEFINE_GUID(IID_ID3D11LinkingNode, 
0xd80dd70c, 0x8d2f, 0x4751, 0x94, 0xa1, 0x3, 0xc7, 0x9b, 0x35, 0x56, 0xdb);

#undef INTERFACE
#define INTERFACE ID3D11LinkingNode

DECLARE_INTERFACE_(ID3D11LinkingNode, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
};


// {54133220-1CE8-43D3-8236-9855C5CEECFF}
interface DECLSPEC_UUID("54133220-1CE8-43D3-8236-9855C5CEECFF") ID3D11FunctionLinkingGraph;
DEFINE_GUID(IID_ID3D11FunctionLinkingGraph, 
0x54133220, 0x1ce8, 0x43d3, 0x82, 0x36, 0x98, 0x55, 0xc5, 0xce, 0xec, 0xff);

#undef INTERFACE
#define INTERFACE ID3D11FunctionLinkingGraph

DECLARE_INTERFACE_(ID3D11FunctionLinkingGraph, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID iid, _Out_ LPVOID * ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Create a shader module out of FLG description.
    STDMETHOD(CreateModuleInstance)(THIS_ _COM_Outptr_ interface ID3D11ModuleInstance ** ppModuleInstance, 
                                          _Always_(_Outptr_opt_result_maybenull_) ID3DBlob ** ppErrorBuffer) PURE;

    STDMETHOD(SetInputSignature)(THIS_ __in_ecount(cInputParameters) const D3D11_PARAMETER_DESC * pInputParameters,
                                       _In_ UINT cInputParameters,
                                       _COM_Outptr_ interface ID3D11LinkingNode ** ppInputNode) PURE;

    STDMETHOD(SetOutputSignature)(THIS_ __in_ecount(cOutputParameters) const D3D11_PARAMETER_DESC * pOutputParameters,
                                        _In_ UINT cOutputParameters,
                                        _COM_Outptr_ interface ID3D11LinkingNode ** ppOutputNode) PURE;

    STDMETHOD(CallFunction)(THIS_ _In_opt_ LPCSTR pModuleInstanceNamespace,
                                  _In_ interface ID3D11Module * pModuleWithFunctionPrototype,
                                  _In_ LPCSTR pFunctionName,
                                  _COM_Outptr_ interface ID3D11LinkingNode ** ppCallNode) PURE;

    STDMETHOD(PassValue)(THIS_ _In_ interface ID3D11LinkingNode * pSrcNode,
                               _In_ INT SrcParameterIndex,
                               _In_ interface ID3D11LinkingNode * pDstNode,
                               _In_ INT DstParameterIndex) PURE;

    STDMETHOD(PassValueWithSwizzle)(THIS_ _In_ interface ID3D11LinkingNode * pSrcNode,
                                          _In_ INT SrcParameterIndex,
                                          _In_ LPCSTR pSrcSwizzle,
                                          _In_ interface ID3D11LinkingNode * pDstNode,
                                          _In_ INT DstParameterIndex,
                                          _In_ LPCSTR pDstSwizzle) PURE;

    STDMETHOD(GetLastError)(THIS_ _Always_(_Outptr_opt_result_maybenull_) ID3DBlob ** ppErrorBuffer) PURE;

    STDMETHOD(GenerateHlsl)(THIS_ _In_ UINT uFlags,                 // uFlags is reserved for future use.
                                  _COM_Outptr_ ID3DBlob ** ppBuffer) PURE;
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
    
#endif //__D3D11SHADER_H__

