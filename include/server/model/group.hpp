#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

// User表的ORM类
class Group
{
public:
    Group()
    {
        this->id = -1;
        this->name = "";
        this->desc = "";
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return id; }
    string getName() { return name; }
    string getDesc() { return desc; }
    // 一定要返回一个引用，因为没有setUsers函数，返回引用才能将信息保存住
    vector<GroupUser> &getUsers() { return users; }

private:
    int id;
    string name;
    string desc;
    // 用来存储一个群组中所有的用户信息
    vector<GroupUser> users;
};

#endif