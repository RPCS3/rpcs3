#pragma once

// Return Codes
enum
{
};

enum
{
	SCE_NP_COMMUNICATION_SIGNATURE_SIZE      = 160,
	SCE_NET_NP_COMMUNICATION_PASSPHRASE_SIZE = 128,
};

// Structs
struct SceNpCommunicationId
{
	char data[9];
	char term;
	u8 num;
	char dummy;
};

struct SceNpCommunicationSignature
{
	uint8_t data[SCE_NP_COMMUNICATION_SIGNATURE_SIZE];
};
