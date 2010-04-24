/*************************************************************************
 *
 *              INTEL CORPORATION PROPRIETARY INFORMATION
 *  This software is supplied under the terms of a license agreement or
 *  nondisclosure agreement with Intel Corporation and may not be copied
 *  or disclosed except in accordance with the terms of that agreement.
 *
 *    Copyright(c) 2008 Intel Corporation. All Rights Reserved.
 *
 ************************************************************************/
#ifndef __JITPROFILING_H__
#define __JITPROFILING_H__

/***************************************
 * Various constants used by functions *
 ***************************************/

 // event notification
 typedef enum iJIT_jvm_event
 {

    // shutdown
   iJVM_EVENT_TYPE_SHUTDOWN = 2,                // Program exiting 
                                                // EventSpecificData NA


    // JIT profiling
   iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED=13,     // issued after method code jitted
                                                // into memory but before code is executed
                                                // EventSpecificData is an iJIT_Method_Load  

   iJVM_EVENT_TYPE_METHOD_UNLOAD_START,         // issued before unload. Method code will no
                                                // longer be executed, but code and info
                                                // are still in memory. The VTune profiler
                                                // may capture method code only at this point.
                                                // EventSpecificData is iJIT_Method_Id

    // Method Profiling

   // method name, Id and stack is supplied 
   iJVM_EVENT_TYPE_ENTER_NIDS = 19,             // issued when a method is about to be entered
                                                // EventSpecificData is iJIT_Method_NIDS

   // method name, Id and stack is supplied 
   iJVM_EVENT_TYPE_LEAVE_NIDS,                  // issued when a method is about to be left
                                                // EventSpecificData is iJIT_Method_NIDS

 } iJIT_JVM_EVENT;

 typedef enum _iJIT_ModeFlags
 {
     iJIT_NO_NOTIFICATIONS             = 0x0000,    // No need to Notify VTune, Since VTune is not running.

     iJIT_BE_NOTIFY_ON_LOAD            = 0x0001,    // when turned on the jit must call iJIT_NotifyEvent( iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, ...) for all the method already jitted.
     iJIT_BE_NOTIFY_ON_UNLOAD          = 0x0002,    // when turned on the jit must call iJIT_NotifyEvent( iJVM_EVENT_TYPE_METHOD_UNLOAD_FINISHED, ...) for all the method that are unloaded

     iJIT_BE_NOTIFY_ON_METHOD_ENTRY    = 0x0004,    // when turned on the jit must instrument all the currently jited code with calls on method entries
     iJIT_BE_NOTIFY_ON_METHOD_EXIT     = 0x0008,    // when turned on the jit must instrument all the currently jited code with calls on method exit
     
 } iJIT_ModeFlags;


 // Flags used by iJIT_IsProfilingActive()
typedef enum _iJIT_IsProfilingActiveFlags
{
 iJIT_NOTHING_RUNNING                   = 0x0000,   // No profiler is running. Currently not used.
 iJIT_SAMPLING_ON                       = 0x0001,   // Sampling is running. This is the default value returned by iJIT_IsProfilingActive()
 iJIT_CALLGRAPH_ON                      = 0x0002    // Call Graph is running

} iJIT_IsProfilingActiveFlags;


 // Enumerator for the environment of methods
typedef enum _iJDEnvironmentType
{
    iJDE_JittingAPI = 2

} iJDEnvironmentType;

/**********************************
 * Data structures for the events *
 **********************************/

 // structure for the events:
 // iJVM_EVENT_TYPE_METHOD_UNLOAD_START   

typedef struct _iJIT_Method_Id 
{
     unsigned int       method_id;              // Id of the method (same as the one passed in the iJIT_Method_Load struct 

} *piJIT_Method_Id, iJIT_Method_Id;


 // structure for the events:
 // iJVM_EVENT_TYPE_ENTER_NIDS,
 // iJVM_EVENT_TYPE_LEAVE_NIDS,
 // iJVM_EVENT_TYPE_EXCEPTION_OCCURRED_NIDS

typedef struct _iJIT_Method_NIDS
{
     unsigned int       method_id;              // unique method ID
     unsigned int       stack_id;               // NOTE: no need to fill this field, it's filled by VTune
     char*              method_name;            // method name (just the method, without the class)

} *piJIT_Method_NIDS, iJIT_Method_NIDS;

 // structures for the events:
 // iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED  

typedef struct _LineNumberInfo 
{
    unsigned int        Offset;                 // x86 Offset from the begining of the method
    unsigned int        LineNumber;             // source line number from the begining of the source file.

} *pLineNumberInfo, LineNumberInfo;

typedef struct _iJIT_Method_Load 
{
    unsigned int        method_id;              // unique method ID - can be any unique value, (except 0 - 999)
    char*               method_name;            // method name (can be with or without the class and signature, in any case the class name will be added to it)
    void*               method_load_address;    // virtual address of that method  - This determines the method range for the iJVM_EVENT_TYPE_ENTER/LEAVE_METHOD_ADDR events
    unsigned int        method_size;            // Size in memory - Must be exact
    unsigned int        line_number_size;       // Line Table size in number of entries - Zero if none
    pLineNumberInfo     line_number_table;      // Pointer to the begining of the line numbers info array
    unsigned int        class_id;               // unique class ID
    char*               class_file_name;        // class file name 
    char*               source_file_name;       // source file name
    void*               user_data;              // bits supplied by the user for saving in the JIT file...
    unsigned int        user_data_size;         // the size of the user data buffer
    iJDEnvironmentType  env;                    // NOTE: no need to fill this field, it's filled by VTune

} *piJIT_Method_Load, iJIT_Method_Load;



//API Functions
#ifdef __cplusplus 
extern "C" {
#endif

typedef void (*iJIT_ModeChangedEx)(void *UserData, iJIT_ModeFlags Flags);  // called when the settings are changed with new settings

int iJIT_NotifyEvent(iJIT_JVM_EVENT event_type, void *EventSpecificData);
void iJIT_RegisterCallbackEx(void *userdata, iJIT_ModeChangedEx NewModeCallBackFuncEx); // The new mode call back routine
iJIT_IsProfilingActiveFlags iJIT_IsProfilingActive();
void FinalizeThread();
void FinalizeProcess();
unsigned int iJIT_GetNewMethodID();

#ifdef __cplusplus 
}
#endif

#endif //__JITPROFILING_H__
