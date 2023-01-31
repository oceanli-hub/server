#ifndef JSON_SCHEMA_INCLUDED
#define JSON_SCHEMA_INCLUDED

/* Copyright (c) 2016, 2021, MariaDB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */


/* This file defines all json schema classes. */

#include "sql_class.h"
#include "sql_type_json.h"
#include "json_schema_helper.h"

class Json_schema_keyword : public Sql_alloc
{
  public:
    Json_schema_keyword *alternate_schema;
    char keyword_name[64];
    double value;
    uint priority;
    bool allowed;

    Json_schema_keyword()
    {
      priority= 0;
      alternate_schema= NULL;
      value= 0;
      allowed= true;
    }
    virtual ~Json_schema_keyword() = default;

    /*
     Called for each keyword on the current level.
    */
    virtual bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                          const uchar *k_end= NULL)
    { return false; }
    virtual bool handle_keyword(THD *thd, json_engine_t *je,
                                const char* key_start,
                                const char* key_end,
                                List<Json_schema_keyword> *all_keywords)
    {
      return false;
    }
    virtual List<Json_schema_keyword>* get_validation_keywords()
    {
      return NULL;
    }
    void set_alternate_schema(Json_schema_keyword *schema)
    {
      alternate_schema= schema;
    }
    virtual bool fall_back_on_alternate_schema(const json_engine_t *je,
                                               const uchar* k_start= NULL,
                                               const uchar* k_end= NULL);
    virtual bool validate_as_alternate(const json_engine_t *je,
                                               const uchar* k_start= NULL,
                                               const uchar* k_end= NULL)
    {
      return false;
    }
    virtual bool validate_schema_items(const json_engine_t *je,
                                       List<Json_schema_keyword>*schema_items);
    virtual void set_alternate_schema_choice(Json_schema_keyword *schema1,
                                             Json_schema_keyword *schema2)
    {
      return;
    }
    virtual void set_dependents(Json_schema_keyword *schema1,
                                Json_schema_keyword *schema2)
    {
      return;
    }
};

/*
  Additional and unvaluated keywords anf items handle
  keywords and validate schema in same way, so it makes sense
  to have a base clasa for them.
*/
class Json_schema_additional_and_unevaluated : public Json_schema_keyword
{
  public:
    List<Json_schema_keyword> schema_list;
    Json_schema_additional_and_unevaluated()
    {
      allowed= true;
    }
    void set_allowed(bool allowed_val)
    {
      allowed= allowed_val;
    }
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    bool validate(const json_engine_t *je,
                  const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override
    {
      return false;
    }
    bool validate_as_alternate(const json_engine_t *je, const uchar *k_start,
                               const uchar *k_end) override;
};


class Json_schema_annotation : public Json_schema_keyword
{
  public:
    Json_schema_annotation()
    {
      size_t len= strlen("annotation");
      strncpy(keyword_name, (const char*)"annotation", len);
      keyword_name[len]='\0';
    }
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
};

class Json_schema_format : public Json_schema_keyword
{
  public:
    Json_schema_format()
    {
      size_t len= strlen("format");
      strncpy(keyword_name, (const char*)"format", len);
      keyword_name[len]='\0';
    }
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
};

typedef List<Json_schema_keyword> List_schema_keyword;

class Json_schema_type : public Json_schema_keyword
{
  private:
    uint type;

  public:
    bool validate(const json_engine_t *je,
                  const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd,
                        json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_type()
    {
      size_t len= strlen("type");
      strncpy(keyword_name, (const char*)"type", len);
      keyword_name[len]='\0';
      type= 0;
    }
};

class Json_schema_const : public Json_schema_keyword
{
  private:
    char *const_json_value;

  public:
    enum json_value_types type;
    bool validate(const json_engine_t *je,
                  const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_const()
    {
      size_t len= strlen("const");
      strncpy(keyword_name, (const char*)"const", len);
      keyword_name[len]='\0';
      const_json_value= NULL;
    }
};

enum enum_scalar_values {
                         HAS_NO_VAL= 0, HAS_TRUE_VAL= 2,
                         HAS_FALSE_VAL= 4, HAS_NULL_VAL= 8
                        };
class Json_schema_enum : public  Json_schema_keyword
{
  private:
    HASH enum_values;
    uint enum_scalar;

  public:
    bool validate(const json_engine_t *je,
                  const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_enum()
    {
      size_t len= strlen("enum");
      strncpy(keyword_name, (const char*)"enum", len);
      keyword_name[len]='\0';
      enum_scalar= HAS_NO_VAL;
    }
    ~Json_schema_enum()
    {
      my_hash_free(&enum_values);
    }
};

class Json_schema_maximum : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je,
                  const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_maximum()
    {
      size_t len= strlen("maximum");
      strncpy(keyword_name, (const char*)"maximum", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_minimum : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je,
                  const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_minimum()
    {
      size_t len= strlen("minimum");
      strncpy(keyword_name, (const char*)"minimum", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_multiple_of : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_multiple_of()
    {
      size_t len= strlen("multiple_of");
      strncpy(keyword_name, (const char*)"multiple_of", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_ex_maximum : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_ex_maximum()
    {
      size_t len= strlen("exclusiveMaximum");
      strncpy(keyword_name, (const char*)"exclusiveMaximum", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_ex_minimum : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_ex_minimum()
    {
      size_t len= strlen("exclusiveMinimum");
      strncpy(keyword_name, (const char*)"exclusiveMinimum", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_max_len : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_max_len()
    {
      size_t len= strlen("maxLength");
      strncpy(keyword_name, (const char*)"maxLength", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_min_len : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_min_len()
    {
      size_t len= strlen("minLength");
      strncpy(keyword_name, (const char*)"minLength", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_pattern : public Json_schema_keyword
{
  private:
    Regexp_processor_pcre re;
    Item *pattern;
    Item_string *str;

  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_pattern()
    {
      str= NULL;
      pattern= NULL;
      size_t len= strlen("pattern");
      strncpy(keyword_name, (const char*)"pattern", len);
      keyword_name[len]='\0';
    }
    ~Json_schema_pattern() { re.cleanup(); }
};

class Json_schema_max_items : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_max_items()
    {
      size_t len= strlen("maxItems");
      strncpy(keyword_name, (const char*)"maxItems", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_min_items : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_min_items()
    {
      size_t len= strlen("minItems");
      strncpy(keyword_name, (const char*)"minItems", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_max_contains : public Json_schema_keyword
{
  public:
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_max_contains()
    {
      size_t len= strlen("maxContains");
      strncpy(keyword_name, (const char*)"maxContains", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_min_contains : public Json_schema_keyword
{
  public:
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_min_contains()
    {
      size_t len= strlen("minContains");
      strncpy(keyword_name, (const char*)"minContains", len);
      keyword_name[len]='\0';
    }
};
/*
  The value of max_contains and min_contains is only
  relevant when contains keyword is present.
  Hence the pointers to access them directly.
*/
class Json_schema_contains : public Json_schema_keyword
{
  public:
    List <Json_schema_keyword> contains;
    Json_schema_keyword *max_contains;
    Json_schema_keyword *min_contains;

    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_contains()
    {
      size_t len= strlen("contains");
      strncpy(keyword_name, (const char*)"contains", len);
      keyword_name[len]='\0';
    }
    void set_dependents(Json_schema_keyword *min, Json_schema_keyword *max)
    {
      min_contains= min;
      max_contains= max;
    }
};

class Json_schema_unique_items : public Json_schema_keyword
{
  private:
    bool is_unique;

  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_unique_items()
    {
      size_t len= strlen("uniqueItems");
      strncpy(keyword_name, (const char*)"uniqueItems", len);
      keyword_name[len]='\0';
    }
};


class Json_schema_prefix_items : public Json_schema_keyword
{
  public:
    List <List_schema_keyword> prefix_items;
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_prefix_items()
    {
      size_t len= strlen("prefixItems");
      strncpy(keyword_name, (const char*)"prefixItems", len);
      keyword_name[len]='\0';
      priority= 1;
    }
};

class Json_schema_unevaluated_items :
      public Json_schema_additional_and_unevaluated
{
  public:
    Json_schema_unevaluated_items()
    {
      priority= 4;
      size_t len= strlen("unevaluatedItems");
      strncpy(keyword_name, (const char*)"unevaluatedItems", len);
      keyword_name[len]='\0';
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
};

class Json_schema_additional_items :
      public Json_schema_additional_and_unevaluated
{
  public:
    Json_schema_additional_items()
    {
      priority= 3;
      size_t len= strlen("additionalItems");
      strncpy(keyword_name, (const char*)"additionalItems", len);
      keyword_name[len]='\0';
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
};

class Json_schema_items : public Json_schema_keyword
{
  private:
    List<Json_schema_keyword> items_schema;
  public:
    Json_schema_items()
    {
      size_t len= strlen("items");
      strncpy(keyword_name, (const char*)"items", len);
      keyword_name[len]='\0';
      priority= 2;
    }
    void set_allowed(bool allowed_val) { allowed= allowed_val; }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    bool validate_as_alternate(const json_engine_t *je, const uchar *k_start,
                           const uchar *k_end) override;
};


class Json_schema_property_names : public Json_schema_keyword
{
  protected:
    List <Json_schema_keyword> property_names;

  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_property_names()
    {
      size_t len= strlen("propertyNames");
      strncpy(keyword_name, (const char*)"propertyNames", len);
      keyword_name[len]='\0';
    }
};

typedef struct property
{
  List<Json_schema_keyword> *curr_schema;
  char *key_name;
} st_property;

class Json_schema_properties : public Json_schema_keyword
{
  private:
    HASH properties;
    bool is_hash_inited;

  public:
    Json_schema_properties()
    {
      size_t len= strlen("properties");
      strncpy(keyword_name, (const char*)"properties", len);
      keyword_name[len]='\0';
      priority= 1;
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    ~Json_schema_properties()
    {
      if (is_hash_inited)
        my_hash_free(&properties);
    }
    bool validate_as_alternate(const json_engine_t *je, const uchar *k_start,
                               const uchar *k_end) override;
  };

class Json_schema_dependent_schemas : public Json_schema_keyword
{
  private:
    HASH properties;
    bool is_hash_inited;

  public:
    Json_schema_dependent_schemas()
    {
      size_t len= strlen("dependentSchemas");
      strncpy(keyword_name, (const char*)"dependentSchema", len);
      keyword_name[len]='\0';
    }
    ~Json_schema_dependent_schemas()
    {
      if (is_hash_inited)
       my_hash_free(&properties);
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
};


class Json_schema_additional_properties :
      public Json_schema_additional_and_unevaluated
{
  public:
    Json_schema_additional_properties()
    {
      priority= 3;
      size_t len= strlen("additionalProperties");
      strncpy(keyword_name, (const char*)"additionalProperties", len);
      keyword_name[len]='\0';
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
};

class Json_schema_unevaluated_properties :
      public Json_schema_additional_and_unevaluated
{
  public:
    Json_schema_unevaluated_properties()
    {
      priority= 4;
      size_t len= strlen("unevaluatedProperties");
      strncpy(keyword_name, (const char*)"unevaluatedProperties", len);
      keyword_name[len]='\0';
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
};

typedef struct pattern_to_property : public Sql_alloc
{
  Regexp_processor_pcre re;
  Item *pattern;
  List<Json_schema_keyword> *curr_schema;
}st_pattern_to_property;

class Json_schema_pattern_properties : public Json_schema_keyword
{
  private:
    Item_string *str;
    List<st_pattern_to_property> pattern_properties;

  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_pattern_properties()
    {
      size_t len= strlen("patternProperties");
      strncpy(keyword_name, (const char*)"patternProperties", len);
      keyword_name[len]='\0';
      priority= 2;
    }
    ~Json_schema_pattern_properties()
    {
      str= NULL;
      if (!pattern_properties.is_empty())
      {
        st_pattern_to_property *curr_pattern_to_property= NULL;
        List_iterator<st_pattern_to_property> it(pattern_properties);
        while((curr_pattern_to_property= it++))
        {
          curr_pattern_to_property->re.cleanup();
          curr_pattern_to_property->pattern= NULL;
          delete curr_pattern_to_property;
          curr_pattern_to_property= nullptr;
        }
      }
    }
    bool validate_as_alternate(const json_engine_t *je, const uchar *k_start,
                               const uchar *k_end) override;
};


class Json_schema_max_prop : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_max_prop()
    {
      size_t len= strlen("maxProperties");
      strncpy(keyword_name, (const char*)"maxProperties", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_min_prop : public Json_schema_keyword
{
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_min_prop()
    {
      size_t len= strlen("minProperties");
      strncpy(keyword_name, (const char*)"minProperties", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_required : public Json_schema_keyword
{
  private:
    List <String> required_properties;

  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_required()
    {
      size_t len= strlen("required");
      strncpy(keyword_name, (const char*)"required", len);
      keyword_name[len]='\0';
    }
};

typedef struct dependent_keyowrds
{
  String *property;
  List <String> dependents;
} st_dependent_keywords;

class Json_schema_dependent_prop : public Json_schema_keyword
{
  private:
    List<st_dependent_keywords> dependent_required;

  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_dependent_prop()
    {
      size_t len= strlen("dependentProperties");
      strncpy(keyword_name, (const char*)"dependentProperties", len);
      keyword_name[len]='\0';
    }
};

enum logic_enum { HAS_ALL_OF= 2, HAS_ANY_OF= 4, HAS_ONE_OF= 8, HAS_NOT= 16};
class Json_schema_logic : public Json_schema_keyword
{
  protected:
    uint logic_flag;
    List <List_schema_keyword> schema_items;
    Json_schema_keyword *alternate_choice1, *alternate_choice2;
  public:
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    Json_schema_logic()
    {
      size_t len= strlen("logic");
      strncpy(keyword_name, (const char*)"logic", len);
      keyword_name[len]='\0';
      logic_flag= 0;
      alternate_choice1= alternate_choice2= NULL;
    }
    virtual bool validate_count(uint* count, uint* total) { return false; }
    void set_alternate_schema_choice(Json_schema_keyword *schema1,
                                     Json_schema_keyword* schema2) override
    {
      alternate_choice1= schema1;
      alternate_choice2= schema2;
    }
    bool check_validation(const json_engine_t *je, const uchar *k_start= NULL,
                          const uchar *k_end= NULL);
};

class Json_schema_not : public Json_schema_logic
{
  private:
  List <Json_schema_keyword> schema_list;
  public:
    Json_schema_not()
    {
      size_t len= strlen("not");
      strncpy(keyword_name, (const char*)"not", len);
      keyword_name[len]='\0';
      logic_flag= HAS_NOT;
    }
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    bool validate_count(uint *count, uint *total) override
    {
      return *count !=0;
    }
};

class Json_schema_one_of : public Json_schema_logic
{
  public:
    Json_schema_one_of()
    {
      size_t len= strlen("oneOf");
      strncpy(keyword_name, (const char*)"oneOf", len);
      keyword_name[len]='\0';
      logic_flag= HAS_ONE_OF;
    }
    bool validate_count(uint *count, uint *total) override
    {
      return !(*count == 1);
    }
};

class Json_schema_any_of : public Json_schema_logic
{
  public:
    Json_schema_any_of()
    {
      size_t len= strlen("anyOf");
      strncpy(keyword_name, (const char*)"anyOf", len);
      keyword_name[len]='\0';
      logic_flag= HAS_ANY_OF;
    }
    bool validate_count(uint *count, uint *total) override
    {
      return *count == 0;
    }
};

class Json_schema_all_of : public Json_schema_logic
{
  public:
    Json_schema_all_of()
    {
      size_t len= strlen("allOf");
      strncpy(keyword_name, (const char*)"allOf", len);
      keyword_name[len]='\0';
      logic_flag= HAS_ALL_OF;
    }
    bool validate_count(uint *count, uint *total) override
    {
      return *count != *total;
    }
};

class Json_schema_conditional : public Json_schema_keyword
{
  private:
    Json_schema_keyword *if_cond, *else_cond, *then_cond;

  public:
    List<Json_schema_keyword> conditions_schema;
    Json_schema_conditional()
    {
      if_cond= NULL;
      then_cond= NULL;
      else_cond= NULL;
    }
    bool validate(const json_engine_t *je, const uchar *k_start= NULL,
                  const uchar *k_end= NULL) override;
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
    void set_conditions(Json_schema_keyword *if_val,
                        Json_schema_keyword* then_val,
                        Json_schema_keyword *else_val)
    {
      if_cond= if_val;
      then_cond= then_val;
      else_cond= else_val;
    }
    List<Json_schema_keyword>* get_validation_keywords() override
    {
      return &conditions_schema;
    }

};

class Json_schema_if : public Json_schema_conditional
{
  public:
    Json_schema_if()
    {
      size_t len= strlen("if");
      strncpy(keyword_name, (const char*)"if", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_else : public Json_schema_conditional
{

  public:
    Json_schema_else()
    {
      size_t len= strlen("else");
      strncpy(keyword_name, (const char*)"else", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_then : public Json_schema_conditional
{
  public:
    Json_schema_then()
    {
      size_t len= strlen("then");
      strncpy(keyword_name, (const char*)"then", len);
      keyword_name[len]='\0';
    }
};

class Json_schema_media_string : public Json_schema_keyword
{
  public:
    Json_schema_media_string()
    {
      size_t len= strlen("media_string");
      strncpy(keyword_name, (const char*)"media_string", len);
      keyword_name[len]='\0';
    }
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
};

class Json_schema_reference : public Json_schema_keyword
{
  public:
    Json_schema_reference()
    {
      size_t len= strlen("reference");
      strncpy(keyword_name, (const char*)"reference", len);
      keyword_name[len]='\0';
    }
    bool handle_keyword(THD *thd, json_engine_t *je,
                        const char* key_start,
                        const char* key_end,
                        List<Json_schema_keyword> *all_keywords) override;
};

bool create_object_and_handle_keyword(THD *thd, json_engine_t *je,
                                      List<Json_schema_keyword> *keyword_list,                           
                                      List<Json_schema_keyword> *all_keywords);
uchar* get_key_name_for_property(const char *key_name, size_t *length,
                    my_bool /* unused */);
uchar* get_key_name_for_func(const char *key_name, size_t *length,
                    my_bool /* unused */);

typedef struct st_json_schema_keyword_map
{
  LEX_CSTRING func_name;
  Json_schema_keyword*(*func)(THD*);
} json_schema_keyword_map;

bool setup_keyword_hash(json_engine_t *je);
void cleanup_keyword_hash();

#endif
