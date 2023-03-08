#pragma once
#include <string>
#include "tinyorm/reflection.h"
#include "tinyorm/mysql4cpp/timestamp.h"

entity(UserInfo) {

	tableName(user_info);

	column(id, int, tags(name = ID, type = integer, pk auto_increment));
	column(nickname, std::string, tags(name = nickname, type = varchar(50), default ""));
    column(score, int, tags(name = score, type = integer, default 0));
	column(createTime, ::Timestamp, tags(name = create_time, type = timestamp, default NOW()));
    add_unique(uni_name, nickname);
    UserInfo()= default;
};
