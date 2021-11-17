if(NOT TARGET SQLiteCpp)
	find_dependency(SQLiteCpp CONFIG)
endif()

if(NOT TARGET fmt::fmt)
	find_dependency(fmt CONFIG)
endif()

if(NOT TARGET xxHash::xxhash)
	find_dependency(xxHash CONFIG)
endif()