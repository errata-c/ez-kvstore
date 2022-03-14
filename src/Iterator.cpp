#include "KVPrivate.hpp"

#include <cassert>
#include <iostream>
#include <fmt/core.h>

namespace ez {
	ElementIterator::ElementIterator(SQLite::Database& db, std::string_view tableID)
		: stmt(
			db,
			fmt::format(
				"SELECT \"key\", \"value\" FROM \"{}\";",
				tableID
			)
		)
	{
		stmt.executeStep();
	}

	bool ElementIterator::advance() {
		return stmt.executeStep();
	}
	bool ElementIterator::atEnd() {
		return !stmt.hasRow();
	}

	std::string ElementIterator::key() {
		SQLite::Column col = stmt.getColumn(0);

		return std::string(
			(const char*)col.getBlob(),
			col.getBytes()
		);
	}
	std::string ElementIterator::value() {
		SQLite::Column col = stmt.getColumn(1);

		return std::string(
			(const char*)col.getBlob(),
			col.getBytes()
		);
	}


	TableIterator::TableIterator(SQLite::Database& db)
		: stmt(
			db,
			"SELECT \"name\" FROM \"ez_kvstore_tables\";"
		)
	{
		stmt.executeStep();
	}

	bool TableIterator::advance() {
		return stmt.executeStep();
	}
	bool TableIterator::atEnd() {
		return stmt.hasRow();
	}

	std::string TableIterator::name() {
		SQLite::Column col = stmt.getColumn(0);

		return std::string(
			(const char*)col.getBlob(),
			col.getBytes()
		);
	}


	ElementIterator* KVPrivate::elementIterator() {
		if (db) {
			return new ElementIterator(db.value(), tableID);
		}
		else {
			return nullptr;
		}
	}
	TableIterator* KVPrivate::tableIterator() {
		if (db) {
			return new TableIterator(db.value());
		}
		else {
			return nullptr;
		}
	}
}