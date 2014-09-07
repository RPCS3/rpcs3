#pragma once

void MsgDialogCreate(u32 type, const char* msg, u64& status);
void MsgDialogDestroy();
void MsgDialogProgressBarSetMsg(u32 index, const char* msg);
void MsgDialogProgressBarReset(u32 index);
void MsgDialogProgressBarInc(u32 index, u32 delta);
