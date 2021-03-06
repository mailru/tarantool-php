#ifndef   PHP_TNT_SCHEMA_H
#define   PHP_TNT_SCHEMA_H

#include <stdint.h>

struct schema_key {
	const char *id;
	uint32_t id_len;
	uint32_t number;
};

/**
 * Possible field data types.
 */
enum field_type {
	FIELD_TYPE_ANY = 0,
	FIELD_TYPE_UNSIGNED,
	FIELD_TYPE_STRING,
	FIELD_TYPE_NUMBER,
	FIELD_TYPE_DOUBLE,
	FIELD_TYPE_INTEGER,
	FIELD_TYPE_BOOLEAN,
	FIELD_TYPE_VARBINARY,
	FIELD_TYPE_SCALAR,
	FIELD_TYPE_DECIMAL,
	FIELD_TYPE_UUID,
	FIELD_TYPE_ARRAY,
	FIELD_TYPE_MAP,
	/* Used for unknown type. */
	field_type_MAX
};

#define COLL_NONE UINT32_MAX

struct schema_field_value {
	uint32_t        field_number;
	uint32_t        field_name_len;
	char           *field_name;
	enum field_type field_type;
	/* Collation ID for string comparison. */
	uint32_t        coll_id;
	/* True if a key part can store NULLs. */
	bool            is_nullable;
};

struct schema_index_value {
	struct schema_key          key;
	char                      *index_name;
	uint32_t                   index_name_len;
	uint32_t                   index_number;
	struct schema_field_value *index_parts;
	uint32_t                   index_parts_len;
};

struct mh_schema_index_t;

struct schema_space_value {
	struct schema_key          key;
	char                      *space_name;
	uint32_t                   space_name_len;
	uint32_t                   space_number;
	struct mh_schema_index_t  *index_hash;
	struct schema_field_value *schema_list;
	uint32_t                   schema_list_len;
};

struct mh_schema_space_t;

struct tarantool_schema {
	struct mh_schema_space_t *space_hash;
};

int
tarantool_schema_add_spaces(struct tarantool_schema *, const char *, uint32_t);
int
tarantool_schema_add_indexes(struct tarantool_schema *, const char *, uint32_t);

int32_t
tarantool_schema_get_sid_by_string(struct tarantool_schema *, const char *,
				   uint32_t);
int32_t
tarantool_schema_get_sid_by_number(struct tarantool_schema *, uint32_t);
int32_t
tarantool_schema_get_iid_by_string(struct tarantool_schema *, uint32_t,
				   const char *, uint32_t);
int32_t
tarantool_schema_get_fid_by_string(struct tarantool_schema *, uint32_t,
				   const char *, uint32_t);

struct tarantool_schema *
tarantool_schema_new(int is_persistent);
void
tarantool_schema_flush(struct tarantool_schema *);
void
tarantool_schema_delete(struct tarantool_schema *, int is_persistent);

#endif /* PHP_TNT_SCHEMA_H */
