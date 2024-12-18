#pragma once

#include "util/types.hpp"

#include <map>
#include <string>

// Do not confuse this with the "user" in Emu/System.h.
// That user is read from config.yml, and it only represents the currently "logged in" user.
// The user_account class will represent all users in the home directory for the User Manager dialog.
// Selecting a user account in this dialog and saving writes it to config.yml.
class user_account
{
public:
	explicit user_account(const std::string& user_id = "00000001");

	const std::string& GetUserId() const { return m_user_id; }
	const std::string& GetUserDir() const { return m_user_dir; }
	const std::string& GetUsername() const { return m_username; }

	static std::map<u32, user_account> GetUserAccounts(const std::string& base_dir);

private:
	std::string m_user_id;
	std::string m_user_dir;
	std::string m_username;
};
