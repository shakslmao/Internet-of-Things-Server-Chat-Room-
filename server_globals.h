#ifndef SERVER_GLOBALS_H
#define SERVER_GLOBALS_H

#include <map>
#include <string>
#include <vector>

struct sockaddr_in;

typedef std::map<std::string, sockaddr_in *> online_users;
typedef std::map<std::string, std::vector<std::string>> group_members;
typedef std::map<std::string, std::string> user_group_map;

extern online_users g_online_users;
extern group_members g_groups;
extern user_group_map g_user_groups;

#endif
