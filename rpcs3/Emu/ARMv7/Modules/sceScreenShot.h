#pragma once

struct SceScreenShotParam
{
	vm::lcptr<char> photoTitle;
	vm::lcptr<char> gameTitle;
	vm::lcptr<char> gameComment;
	vm::lptr<void> reserved;
};
