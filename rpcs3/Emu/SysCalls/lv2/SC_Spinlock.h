#pragma once

struct spinlock
{
	SMutexBE mutex;
};