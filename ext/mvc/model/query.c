
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  |          ZhuZongXin <dreamsxin@qq.com>                                 |
  |          Kenji Minamoto <kenji.minamoto@gmail.com>                     |
  +------------------------------------------------------------------------+
*/

#include "mvc/model/query.h"
#include "mvc/model/queryinterface.h"
#include "mvc/model/query/scanner.h"
#include "mvc/model/query/phql.h"
#include "mvc/model/query/status.h"
#include "mvc/model/resultset/complex.h"
#include "mvc/model/resultset/simple.h"
#include "mvc/model/query/exception.h"
#include "mvc/model/manager.h"
#include "mvc/model/managerinterface.h"
#include "mvc/model/metadatainterface.h"
#include "mvc/model/metadata/memory.h"
#include "mvc/model/row.h"
#include "cache/backendinterface.h"
#include "cache/frontendinterface.h"
#include "diinterface.h"
#include "di/injectable.h"
#include "db/rawvalue.h"
#include "db/column.h"
#include "db/adapterinterface.h"
#include "debug.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/exception.h"
#include "kernel/array.h"
#include "kernel/concat.h"
#include "kernel/operators.h"
#include "kernel/string.h"
#include "kernel/framework/orm.h"
#include "kernel/hash.h"
#include "kernel/file.h"
#include "kernel/debug.h"

#include "interned-strings.h"

/**
 * Phalcon\Mvc\Model\Query
 *
 * This class takes a PHQL intermediate representation and executes it.
 *
 *<code>
 *
 * $phql = "SELECT c.price*0.16 AS taxes, c.* FROM Cars AS c JOIN Brands AS b
 *          WHERE b.name = :name: ORDER BY c.name";
 *
 * $result = $manager->executeQuery($phql, array(
 *   'name' => 'Lamborghini'
 * ));
 *
 * foreach ($result as $row) {
 *   echo "Name: ", $row->cars->name, "\n";
 *   echo "Price: ", $row->cars->price, "\n";
 *   echo "Taxes: ", $row->taxes, "\n";
 * }
 *
 *</code>
 */
zend_class_entry *phalcon_mvc_model_query_ce;

PHP_METHOD(Phalcon_Mvc_Model_Query, __construct);
PHP_METHOD(Phalcon_Mvc_Model_Query, setPhql);
PHP_METHOD(Phalcon_Mvc_Model_Query, getPhql);
PHP_METHOD(Phalcon_Mvc_Model_Query, getModelsManager);
PHP_METHOD(Phalcon_Mvc_Model_Query, getModelsMetaData);
PHP_METHOD(Phalcon_Mvc_Model_Query, setUniqueRow);
PHP_METHOD(Phalcon_Mvc_Model_Query, getUniqueRow);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getQualified);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getCallArgument);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getCaseExpression);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getFunctionCall);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getExpression);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getSelectColumn);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getTable);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getJoin);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getJoinType);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getSingleJoin);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getMultiJoin);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getJoins);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getOrderClause);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getGroupClause);
PHP_METHOD(Phalcon_Mvc_Model_Query, _getLimitClause);
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareSelect);
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareInsert);
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareUpdate);
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareDelete);
PHP_METHOD(Phalcon_Mvc_Model_Query, parse);
PHP_METHOD(Phalcon_Mvc_Model_Query, cache);
PHP_METHOD(Phalcon_Mvc_Model_Query, getCacheOptions);
PHP_METHOD(Phalcon_Mvc_Model_Query, getCache);
PHP_METHOD(Phalcon_Mvc_Model_Query, ignoreLastInsertId);
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeSelect);
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeInsert);
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeUpdate);
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeDelete);
PHP_METHOD(Phalcon_Mvc_Model_Query, execute);
PHP_METHOD(Phalcon_Mvc_Model_Query, getSingleResult);
PHP_METHOD(Phalcon_Mvc_Model_Query, setType);
PHP_METHOD(Phalcon_Mvc_Model_Query, getType);
PHP_METHOD(Phalcon_Mvc_Model_Query, getBindParam);
PHP_METHOD(Phalcon_Mvc_Model_Query, setBindParams);
PHP_METHOD(Phalcon_Mvc_Model_Query, getBindParams);
PHP_METHOD(Phalcon_Mvc_Model_Query, setMergeBindParams);
PHP_METHOD(Phalcon_Mvc_Model_Query, getMergeBindParams);
PHP_METHOD(Phalcon_Mvc_Model_Query, setBindType);
PHP_METHOD(Phalcon_Mvc_Model_Query, setBindTypes);
PHP_METHOD(Phalcon_Mvc_Model_Query, getBindTypes);
PHP_METHOD(Phalcon_Mvc_Model_Query, setMergeBindTypes);
PHP_METHOD(Phalcon_Mvc_Model_Query, getMergeBindTypes);
PHP_METHOD(Phalcon_Mvc_Model_Query, setIndex);
PHP_METHOD(Phalcon_Mvc_Model_Query, getIndex);
PHP_METHOD(Phalcon_Mvc_Model_Query, setIntermediate);
PHP_METHOD(Phalcon_Mvc_Model_Query, getIntermediate);
PHP_METHOD(Phalcon_Mvc_Model_Query, getModels);
PHP_METHOD(Phalcon_Mvc_Model_Query, setConnection);
PHP_METHOD(Phalcon_Mvc_Model_Query, getConnection);
PHP_METHOD(Phalcon_Mvc_Model_Query, getReadConnection);
PHP_METHOD(Phalcon_Mvc_Model_Query, getWriteConnection);
PHP_METHOD(Phalcon_Mvc_Model_Query, setConflict);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, phql)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setphql, 0, 1, 0)
	ZEND_ARG_INFO(0, phql)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setuniquerow, 0, 0, 1)
	ZEND_ARG_INFO(0, uniqueRow)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_cache, 0, 0, 1)
	ZEND_ARG_INFO(0, cacheOptions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_getsingleresult, 0, 0, 0)
	ZEND_ARG_INFO(0, bindParams)
	ZEND_ARG_INFO(0, bindTypes)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_settype, 0, 0, 1)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_getbindparam, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setbindparams, 0, 0, 1)
	ZEND_ARG_INFO(0, bindParams)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setmergebindparams, 0, 0, 1)
	ZEND_ARG_INFO(0, bindParams)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setbindtype, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setbindtypes, 0, 0, 1)
	ZEND_ARG_INFO(0, bindTypes)
ZEND_END_ARG_INFO()
/*
ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setmergebindtypes, 0, 0, 1)
	ZEND_ARG_INFO(0, bindTypes)
ZEND_END_ARG_INFO()
*/
ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setindex, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, index, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setintermediate, 0, 0, 1)
	ZEND_ARG_INFO(0, intermediate)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setconnection, 0, 0, 1)
	ZEND_ARG_INFO(0, connection)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_getreadconnection, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, intermediate, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, bindParams, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, bindTypes, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_getwriteconnection, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, intermediate, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, bindParams, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, bindTypes, IS_ARRAY, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_model_query_setconflict, 0, 0, 1)
	ZEND_ARG_INFO(0, conflict)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_mvc_model_query_method_entry[] = {
	PHP_ME(Phalcon_Mvc_Model_Query, __construct, arginfo_phalcon_mvc_model_query___construct, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Phalcon_Mvc_Model_Query, setPhql, arginfo_phalcon_mvc_model_query_setphql, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getPhql, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getModelsManager, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getModelsMetaData, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setUniqueRow, arginfo_phalcon_mvc_model_query_setuniquerow, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getUniqueRow, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, _getQualified, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getCallArgument, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getCaseExpression, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getFunctionCall, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getExpression, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getSelectColumn, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getTable, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getJoin, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getJoinType, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getSingleJoin, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getMultiJoin, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getJoins, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getOrderClause, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getGroupClause, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _getLimitClause, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _prepareSelect, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _prepareInsert, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _prepareUpdate, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _prepareDelete, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, parse, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, cache, arginfo_phalcon_mvc_model_query_cache, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getCacheOptions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getCache, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, ignoreLastInsertId, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, _executeSelect, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _executeInsert, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _executeUpdate, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, _executeDelete, NULL, ZEND_ACC_PROTECTED)
	PHP_ME(Phalcon_Mvc_Model_Query, execute, arginfo_phalcon_mvc_model_queryinterface_execute, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getSingleResult, arginfo_phalcon_mvc_model_query_getsingleresult, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setType, arginfo_phalcon_mvc_model_query_settype, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getType, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getBindParam, arginfo_phalcon_mvc_model_query_getbindparam, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setBindParams, arginfo_phalcon_mvc_model_query_setbindparams, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getBindParams, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setMergeBindParams, arginfo_phalcon_mvc_model_query_setmergebindparams, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getMergeBindParams, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setBindType, arginfo_phalcon_mvc_model_query_setbindtype, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setBindTypes, arginfo_phalcon_mvc_model_query_setbindtypes, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getBindTypes, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setMergeBindTypes, arginfo_phalcon_mvc_model_query_setbindtypes, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getMergeBindTypes, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setIndex, arginfo_phalcon_mvc_model_query_setindex, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getIndex, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setIntermediate, arginfo_phalcon_mvc_model_query_setintermediate, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getIntermediate, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getModels, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setConnection, arginfo_phalcon_mvc_model_query_setconnection, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getConnection, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getReadConnection, arginfo_phalcon_mvc_model_query_getreadconnection, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, getWriteConnection, arginfo_phalcon_mvc_model_query_getwriteconnection, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Model_Query, setConflict, arginfo_phalcon_mvc_model_query_setconflict, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Mvc\Model\Query initializer
 */
PHALCON_INIT_CLASS(Phalcon_Mvc_Model_Query){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Mvc\\Model, Query, mvc_model_query, phalcon_di_injectable_ce, phalcon_mvc_model_query_method_entry, 0);

	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_modelsManager"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_modelsMetaData"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_connection"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_type"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_phql"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_ast"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_intermediate"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_models"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_sqlAliases"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_sqlAliasesModels"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_sqlModelsAliases"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_sqlAliasesModelsInstances"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_sqlColumnAliases"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_modelsInstances"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_currentModelsInstances"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_cache"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_cacheOptions"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_uniqueRow"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_bindParams"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_bindTypes"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_mergeBindParams"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_mergeBindTypes"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_index"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_model_query_ce, SL("_ignoreLastInsertId"), ZEND_ACC_PROTECTED);

	zend_declare_class_constant_long(phalcon_mvc_model_query_ce, SL("TYPE_SELECT"), PHQL_T_SELECT);
	zend_declare_class_constant_long(phalcon_mvc_model_query_ce, SL("TYPE_INSERT"), PHQL_T_INSERT);
	zend_declare_class_constant_long(phalcon_mvc_model_query_ce, SL("TYPE_UPDATE"), PHQL_T_UPDATE);
	zend_declare_class_constant_long(phalcon_mvc_model_query_ce, SL("TYPE_DELETE"), PHQL_T_DELETE);

	zend_class_implements(phalcon_mvc_model_query_ce, 1, phalcon_mvc_model_queryinterface_ce);

	return SUCCESS;
}

/**
 * Phalcon\Mvc\Model\Query constructor
 *
 * @param string $phql
 * @param Phalcon\DiInterface $dependencyInjector
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, __construct){

	zval *phql = NULL, *dependency_injector = NULL;

	phalcon_fetch_params(0, 0, 2, &phql, &dependency_injector);

	if (phql && Z_TYPE_P(phql) != IS_NULL) {
		phalcon_update_property(getThis(), SL("_phql"), phql);
	}

	if (dependency_injector && Z_TYPE_P(dependency_injector) == IS_OBJECT) {
		PHALCON_CALL_METHOD(NULL, getThis(), "setdi", dependency_injector);
	}
}

PHP_METHOD(Phalcon_Mvc_Model_Query, setPhql){

	zval *phql;

	phalcon_fetch_params(0, 1, 0, &phql);

	phalcon_update_property(getThis(), SL("_phql"), phql);
}

PHP_METHOD(Phalcon_Mvc_Model_Query, getPhql){


	RETURN_MEMBER(getThis(), "_phql");
}

/**
 * Returns the models manager related to the entity instance
 *
 * @return Phalcon\Mvc\Model\ManagerInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getModelsManager){

	zval manager = {}, service_name = {};

	phalcon_read_property(&manager, getThis(), SL("_modelsManager"), PH_NOISY|PH_READONLY);
	if (Z_TYPE(manager) == IS_OBJECT) {
		RETURN_CTOR(&manager);
	} else {
		/**
		 * Obtain the models-metadata service from the DI
		 */
		ZVAL_STR(&service_name, IS(modelsManager));
		PHALCON_CALL_METHOD(return_value, getThis(), "getresolveservice", &service_name);

		if (Z_TYPE_P(return_value) != IS_OBJECT) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "The injected service 'modelsMetadata' is not valid");
			return;
		}

		PHALCON_VERIFY_INTERFACE(return_value, phalcon_mvc_model_managerinterface_ce);
	}
}

/**
 * Returns the models meta-data service related to the entity instance
 *
 * @return Phalcon\Mvc\Model\MetaDataInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getModelsMetaData){

	zval meta_data = {}, dependency_injector = {}, service_name = {}, has = {}, service = {};

	PHALCON_MM_INIT();

	phalcon_read_property(&meta_data, getThis(), SL("_modelsMetaData"), PH_NOISY|PH_READONLY);
	if (Z_TYPE(meta_data) == IS_OBJECT) {
		RETURN_MM_CTOR(&meta_data);
	}
	/**
	 * Check if the DI is valid
	 */
	PHALCON_MM_CALL_METHOD(&dependency_injector, getThis(), "getdi");
	PHALCON_MM_ADD_ENTRY(&dependency_injector);
	if (Z_TYPE(dependency_injector) != IS_OBJECT) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "A dependency injector container is required to obtain the services related to the ORM");
		return;
	}

	ZVAL_STR(&service_name, IS(modelsMetadata));

	PHALCON_MM_CALL_METHOD(&has, &dependency_injector, "has", &service_name);
	if (zend_is_true(&has)) {
		/**
		 * Obtain the models-metadata service from the DI
		 */
		PHALCON_MM_CALL_METHOD(&service, &dependency_injector, "getshared", &service_name);
		PHALCON_MM_ADD_ENTRY(&service);
		if (Z_TYPE(service) != IS_OBJECT) {
			PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "The injected service 'modelsMetadata' is not valid");
			return;
		}

		PHALCON_MM_VERIFY_INTERFACE(&service, phalcon_mvc_model_metadatainterface_ce);
	} else {
		object_init_ex(&service, phalcon_mvc_model_metadata_memory_ce);
		PHALCON_MM_ADD_ENTRY(&service);
	}

	/**
	 * Update the models-metada property
	 */
	phalcon_update_property(getThis(), SL("_modelsMetaData"), &service);

	RETURN_MM_CTOR(&service);
}

/**
 * Tells to the query if only the first row in the resultset must be returned
 *
 * @param boolean $uniqueRow
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setUniqueRow){

	zval *unique_row;

	phalcon_fetch_params(0, 1, 0, &unique_row);

	phalcon_update_property(getThis(), SL("_uniqueRow"), unique_row);
	RETURN_THIS();
}

/**
 * Check if the query is programmed to get only the first row in the resultset
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getUniqueRow){


	RETURN_MEMBER(getThis(), "_uniqueRow");
}

/**
 * Replaces the model's name to its source name in a qualifed-name expression
 *
 * @param array $expr
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getQualified){

	zval *expr, column_name = {}, sql_column_aliases = {}, s_qualified = {}, sql_aliases = {}, column_domain = {}, phql = {};
	zval source = {}, exception_message = {}, sql_aliases_models_instances = {}, model = {}, column_map = {}, real_column_name = {};
	zval has_model = {}, models_instances = {}, *model_instance, models = {}, class_name = {};
	long int number = 0;

	phalcon_fetch_params(1, 1, 0, &expr);

	phalcon_array_fetch_string(&column_name, expr, IS(name), PH_NOISY|PH_READONLY);

	phalcon_read_property(&sql_column_aliases, getThis(), SL("_sqlColumnAliases"), PH_NOISY|PH_READONLY);

	/**
	 * Check if the qualified name is a column alias
	 */
	if (phalcon_array_isset(&sql_column_aliases, &column_name)) {
		array_init_size(return_value, 2);
		ZVAL_STR(&s_qualified, IS(qualified));
		phalcon_array_update_string(return_value, IS(type), &s_qualified, PH_COPY);
		phalcon_array_update_string(return_value, IS(name), &column_name, PH_COPY);
		RETURN_MM();
	}

	/**
	 * Check if the qualified name has a domain
	 */
	if (phalcon_array_isset_str(expr, SL("domain"))) {
		phalcon_read_property(&sql_aliases, getThis(), SL("_sqlAliases"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_string(&column_domain, expr, IS(domain), PH_NOISY|PH_READONLY);

		/**
		 * The column has a domain, we need to check if it's an alias
		 */
		if (!phalcon_array_isset_fetch(&source, &sql_aliases, &column_domain, PH_READONLY)) {
			phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

			PHALCON_CONCAT_SVSV(&exception_message, "Unknown model or alias '", &column_domain, "' (1), when preparing: ", &phql);
			PHALCON_MM_ADD_ENTRY(&exception_message);
			PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
		}

		/**
		 * Change the selected column by its real name on its mapped table
		 */
		phalcon_read_property(&sql_aliases_models_instances, getThis(), SL("_sqlAliasesModelsInstances"), PH_NOISY|PH_READONLY);

		/**
		 * We need to model instance to retrieve the reversed column map
		 */
		if (!phalcon_array_isset_fetch(&model, &sql_aliases_models_instances, &column_domain, PH_READONLY)) {
			phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

			PHALCON_CONCAT_SVSV(&exception_message, "There is no model related to model or alias '", &column_domain, "', when executing: ", &phql);
			PHALCON_MM_ADD_ENTRY(&exception_message);
			PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
		}

		PHALCON_MM_CALL_METHOD(&column_map, &model, "getreversecolumnmap");
		PHALCON_MM_ADD_ENTRY(&column_map);

		if (Z_TYPE(column_map) == IS_ARRAY) {
			if (!phalcon_array_isset_fetch(&real_column_name, &column_map, &column_name, PH_READONLY)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSVSV(&exception_message, "Column '", &column_name, "' doesn't belong to the model or alias '", &column_domain, "', when executing: ", &phql);
				PHALCON_MM_ADD_ENTRY(&exception_message);
				PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}
		} else {
			ZVAL_COPY_VALUE(&real_column_name, &column_name);
		}
	} else {
		/**
		 * If the column IR doesn't have a domain, we must check for ambiguities
		 */
		ZVAL_FALSE(&has_model);

		phalcon_read_property(&models_instances, getThis(), SL("_currentModelsInstances"), PH_NOISY|PH_READONLY);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(models_instances), model_instance) {
			zval has_attribute = {};
			/**
			 * Check if the atribute belongs to the current model
			 */
			PHALCON_MM_CALL_METHOD(&has_attribute, model_instance, "hasattribute", &column_name);
			if (zend_is_true(&has_attribute)) {
				++number;
				if (number > 1) {
					phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

					PHALCON_CONCAT_SVSV(&exception_message, "The column '", &column_name, "' is ambiguous, when preparing: ", &phql);
					PHALCON_MM_ADD_ENTRY(&exception_message);
					PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
					return;
				}

				ZVAL_COPY_VALUE(&has_model, model_instance);
			}
		} ZEND_HASH_FOREACH_END();

		/**
		 * After check in every model, the column does not belong to any of the selected
		 * models
		 */
		if (PHALCON_IS_FALSE(&has_model)) {
			phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

			PHALCON_CONCAT_SVSV(&exception_message, "Column '", &column_name, "' doesn't belong to any of the selected models (1), when preparing: ", &phql);
			PHALCON_MM_ADD_ENTRY(&exception_message);
			PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
		}

		/**
		 * Check if the _models property is correctly prepared
		 */
		phalcon_read_property(&models, getThis(), SL("_models"), PH_NOISY|PH_READONLY);
		if (Z_TYPE(models) != IS_ARRAY) {
			PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "The models list was not loaded correctly");
			return;
		}

		/**
		 * Obtain the model's source from the _models list
		 */
		phalcon_get_class(&class_name, &has_model, 0);
		PHALCON_MM_ADD_ENTRY(&class_name);
		if (!phalcon_array_isset_fetch(&source, &models, &class_name, PH_READONLY)) {
			phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

			PHALCON_CONCAT_SVSV(&exception_message, "Can't obtain the model '", &class_name, "' source from the _models list, when preparing: ", &phql);
			PHALCON_MM_ADD_ENTRY(&exception_message);
			PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
		}

		/**
		 * Rename the column
		 */
		PHALCON_MM_CALL_METHOD(&column_map, &has_model, "getreversecolumnmap");
		PHALCON_MM_ADD_ENTRY(&column_map);

		if (Z_TYPE(column_map) == IS_ARRAY) {
			/**
			 * The real column name is in the column map
			 */
			if (phalcon_array_isset(&column_map, &column_name)) {
				phalcon_array_fetch(&real_column_name, &column_map, &column_name, PH_NOISY|PH_READONLY);
			} else {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSV(&exception_message, "Column '", &column_name, "' doesn't belong to any of the selected models (3), when preparing: ", &phql);
				PHALCON_MM_ADD_ENTRY(&exception_message);
				PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}
		} else {
			ZVAL_COPY_VALUE(&real_column_name, &column_name);
		}
	}

	/**
	 * Create an array with the qualified info
	 */
	ZVAL_STR(&s_qualified, IS(qualified));
	array_init_size(return_value, 4);
	phalcon_array_update_string(return_value, IS(type), &s_qualified, PH_COPY);
	phalcon_array_update_string(return_value, IS(domain), &source, PH_COPY);
	phalcon_array_update_string(return_value, IS(name), &real_column_name, PH_COPY);
	phalcon_array_update_string(return_value, IS(balias), &column_name, PH_COPY);
	RETURN_MM();
}

/**
 * Resolves a expression in a single call argument
 *
 * @param array $argument
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getCallArgument){

	zval *argument, argument_type = {}, s_all = {};

	phalcon_fetch_params(0, 1, 0, &argument);

	phalcon_array_fetch_string(&argument_type, argument, IS(type), PH_NOISY|PH_READONLY);
	if (PHALCON_IS_LONG(&argument_type, PHQL_T_STARALL)) {
		ZVAL_STRING(&s_all, ISV(all));
		array_init_size(return_value, 1);
		add_assoc_zval_ex(return_value, ISL(type), &s_all);
		return;
	}

	PHALCON_RETURN_CALL_METHOD(getThis(), "_getexpression", argument);
}

/**
 * Resolves a expression in a single call argument
 *
 * @param array $argument
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getCaseExpression){

	zval *expr, when_clauses, left = {}, right = {}, *when_expr, tmp;

	phalcon_fetch_params(1, 1, 0, &expr);

	array_init(&when_clauses);
	PHALCON_MM_ADD_ENTRY(&when_clauses);

	phalcon_array_fetch_string(&left, expr, IS(left), PH_NOISY|PH_READONLY);
	phalcon_array_fetch_string(&right, expr, IS(right), PH_NOISY|PH_READONLY);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(right), when_expr) {
		zval when_left = {}, when_right = {}, expr_left = {}, expr_right = {}, tmp1 = {};

		/**
		 * Resolving left part of the expression if any
		 */
		if (phalcon_array_isset_fetch_str(&expr_left, when_expr, SL("left"), PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&when_left, getThis(), "_getexpression", &expr_left);
			PHALCON_MM_ADD_ENTRY(&when_left);
		}

		array_init(&tmp1);
		PHALCON_MM_ADD_ENTRY(&tmp1);
		if (phalcon_array_isset_str(when_expr, SL("right"))) {
			/**
			 * Resolving right part of the expression if any
			 */
			if (phalcon_array_isset_fetch_str(&expr_right, when_expr, SL("right"), PH_READONLY)) {
				PHALCON_MM_CALL_METHOD(&when_right, getThis(), "_getexpression", &expr_right);
				PHALCON_MM_ADD_ENTRY(&when_right);
			}

			phalcon_array_update_string_str(&tmp1, IS(type), SL("when"), 0);
			phalcon_array_update_str(&tmp1, SL("when"), &when_left, PH_COPY);
			phalcon_array_update_str(&tmp1, SL("then"), &when_right, PH_COPY);
		} else {
			phalcon_array_update_string_str(&tmp1, IS(type), SL("else"), 0);
			phalcon_array_update_str(&tmp1, SL("expr"), &when_left, PH_COPY);
		}
		phalcon_array_append(&when_clauses, &tmp1, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "_getexpression", &left);

	array_init(return_value);

	phalcon_array_update_string_str(return_value, IS(type), SL("case"), 0);
	phalcon_array_update_str(return_value, SL("expr"), &tmp, PH_COPY);
	phalcon_array_update_str(return_value, SL("when-clauses"), &when_clauses, PH_COPY);
	RETURN_MM();
}

/**
 * Resolves a expression in a single call argument
 *
 * @param array $expr
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getFunctionCall){

	zval *expr, name = {}, arguments = {}, function_args = {}, *argument, argument_expr = {};
	int distinct;

	phalcon_fetch_params(1, 1, 0, &expr);

	phalcon_array_fetch_string(&name, expr, IS(name), PH_NOISY|PH_READONLY);

	array_init_size(return_value, 4);

	phalcon_array_update_string_string(return_value, IS(type), IS(functionCall), PH_COPY);
	phalcon_array_update_string(return_value, IS(name), &name, PH_COPY);

	if (phalcon_array_isset_fetch_str(&arguments, expr, SL("arguments"), PH_READONLY)) {
		distinct = phalcon_array_isset_str(expr, SL("distinct")) ? 1 : 0;
		array_init(&function_args);
		PHALCON_MM_ADD_ENTRY(&function_args);
		if (phalcon_array_isset_long(&arguments, 0)) {
			/**
			 * There are more than one argument
			 */
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL(arguments), argument) {
				PHALCON_MM_CALL_METHOD(&argument_expr, getThis(), "_getcallargument", argument);
				phalcon_array_append(&function_args, &argument_expr, 0);
			} ZEND_HASH_FOREACH_END();

		} else {
			/**
			 * There is only one argument
			 */
			PHALCON_MM_CALL_METHOD(&argument_expr, getThis(), "_getcallargument", &arguments);
			phalcon_array_append(&function_args, &argument_expr, 0);
		}

		phalcon_array_update_string(return_value, IS(arguments), &function_args, PH_COPY);

		if (distinct) {
			add_assoc_bool_ex(return_value, ISL(distinct), distinct);
		}
	}
	RETURN_MM();
}

/**
 * Resolves an expression from its intermediate code into a string
 *
 * @param array $expr
 * @param boolean $quoting
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getExpression){

	zval *expr, *not_quoting = NULL, expr_left = {}, expr_right = {}, left = {}, right = {}, expr_type = {}, expr_value = {}, value = {}, escaped_value = {};
	zval value_parts = {}, value_name = {}, value_type = {}, value_param = {}, placeholder = {}, exception_message = {}, list_items = {}, *expr_list_item, models_instances = {};
	int type, value_times;

	phalcon_fetch_params(0, 1, 1, &expr, &not_quoting);

	if (!not_quoting) {
		not_quoting = &PHALCON_GLOBAL(z_false);
	}

	if (phalcon_array_isset_str(expr, ISL(type))) {

		/**
		 * Every node in the AST has a unique integer type
		 */
		phalcon_array_fetch_string(&expr_type, expr, IS(type), PH_NOISY|PH_READONLY);

		type = phalcon_get_intval(&expr_type);
		if (type != PHQL_T_CASE) {
			/**
			 * Resolving left part of the expression if any
			 */
			if (phalcon_array_isset_fetch_str(&expr_left, expr, SL("left"), PH_READONLY)) {
				PHALCON_CALL_METHOD(&left, getThis(), "_getexpression", &expr_left, not_quoting);
			}

			/**
			 * Resolving right part of the expression if any
			 */
			if (phalcon_array_isset_fetch_str(&expr_right, expr, SL("right"), PH_READONLY)) {
				PHALCON_CALL_METHOD(&right, getThis(), "_getexpression", &expr_right, not_quoting);
			}
		}

		switch (type) {

			case PHQL_T_LESS:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("<"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_EQUALS:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("="), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_GREATER:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL(">"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_NOTEQUALS:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("<>"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_LESSEQUAL:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("<="), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_GREATEREQUAL:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL(">="), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_AND:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("AND"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_OR:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("OR"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_QUALIFIED:
				PHALCON_RETURN_CALL_METHOD(getThis(), "_getqualified", expr);
				break;

			case PHQL_T_ADD:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("+"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_SUB:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("-"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_MUL:
				assert(Z_TYPE(left) >= IS_NULL && Z_TYPE(right) >= IS_NULL);
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("*"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_DIV:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("/"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_MOD:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("%"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_BITWISE_AND:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("&"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_BITWISE_OR:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("|"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_ENCLOSED:
				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("parentheses"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				break;

			case PHQL_T_MINUS:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("unary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("-"), 0);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_INTEGER:
			case PHQL_T_DOUBLE:
				phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);

				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string(return_value, IS(value), &value, PH_COPY);
				break;

			case PHQL_T_HINTEGER:
				phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);

				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string(return_value, IS(value), &value, PH_COPY);
				break;

			case PHQL_T_RAW_QUALIFIED:
				phalcon_array_fetch_str(&value, expr, SL("name"), PH_NOISY|PH_READONLY);

				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string(return_value, IS(value), &value, PH_COPY);
				break;

			case PHQL_T_TRUE:
				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string_str(return_value, IS(value), SL("TRUE"), 0);
				break;

			case PHQL_T_FALSE:
				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string_str(return_value, IS(value), SL("FALSE"), 0);
				break;

			case PHQL_T_STRING:
				phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);
				if (!zend_is_true(not_quoting)) {

					/**
					 * Check if static literals have single quotes and escape them
					 */
					if (phalcon_memnstr_str(&value, SL("'"))) {
						phalcon_orm_singlequotes(&escaped_value, &value);
					} else {
						ZVAL_COPY(&escaped_value, &value);
					}

					PHALCON_CONCAT_SVS(&expr_value, "'", &escaped_value, "'");
					zval_ptr_dtor(&escaped_value);
				} else {
					ZVAL_COPY(&expr_value, &value);
				}

				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string(return_value, IS(value), &expr_value, 0);
				break;

			case PHQL_T_NPLACEHOLDER: {
				zval question_mark = {}, colon = {};

				phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);

				ZVAL_STRING(&question_mark, "?");
				ZVAL_STRING(&colon, ":");

				PHALCON_STR_REPLACE(&placeholder, &question_mark, &colon, &value);
				zval_ptr_dtor(&question_mark);
				zval_ptr_dtor(&colon);

				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("placeholder"), 0);
				phalcon_array_update_string(return_value, IS(value), &placeholder, 0);
				break;
			}

			case PHQL_T_SPLACEHOLDER:
				phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SV(&placeholder, ":", &value);

				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("placeholder"), 0);
				phalcon_array_update_string(return_value, IS(value), &placeholder, 0);
				break;

			case PHQL_T_BPLACEHOLDER:
			{
				zval tmp_type = {};
				int error = 0;
				phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);

				if (phalcon_memnstr_str(&value, SL(":"))) {
					phalcon_fast_explode_str(&value_parts, SL(":"), &value);

					phalcon_array_fetch_long(&value_name, &value_parts, 0, PH_NOISY|PH_READONLY);
					phalcon_array_fetch_long(&value_type, &value_parts, 1, PH_NOISY|PH_READONLY);

					PHALCON_CONCAT_SV(&placeholder, ":", &value_name);

					array_init(return_value);
					phalcon_array_update_string_str(return_value, IS(type), SL("placeholder"), 0);
					phalcon_array_update_string(return_value, IS(value), &placeholder, 0);

					if (phalcon_comparestr_str(&value_type, SL("str"), NULL)) {
						ZVAL_LONG(&tmp_type, PHALCON_DB_COLUMN_BIND_PARAM_STR);
						PHALCON_CALL_METHOD(NULL, getThis(), "setbindtype", &value_name, &tmp_type);
					} else if (phalcon_comparestr_str(&value_type, SL("int"), NULL)) {
						ZVAL_LONG(&tmp_type, PHALCON_DB_COLUMN_BIND_PARAM_INT);
						PHALCON_CALL_METHOD(NULL, getThis(), "setbindtype", &value_name, &tmp_type);
					} else if (phalcon_comparestr_str(&value_type, SL("double"), NULL)) {
						ZVAL_LONG(&tmp_type, PHALCON_DB_COLUMN_BIND_PARAM_DECIMAL);
						PHALCON_CALL_METHOD(NULL, getThis(), "setbindtype", &value_name, &tmp_type);
					} else if (phalcon_comparestr_str(&value_type, SL("bool"), NULL)) {
						ZVAL_LONG(&tmp_type, PHALCON_DB_COLUMN_BIND_PARAM_BOOL);
						PHALCON_CALL_METHOD(NULL, getThis(), "setbindtype", &value_name, &tmp_type);
					} else if (phalcon_comparestr_str(&value_type, SL("blob"), NULL)) {
						ZVAL_LONG(&tmp_type, PHALCON_DB_COLUMN_BIND_PARAM_BLOD);
						PHALCON_CALL_METHOD(NULL, getThis(), "setbindtype", &value_name, &tmp_type);
					} else if (phalcon_comparestr_str(&value_type, SL("null"), NULL)) {
						ZVAL_LONG(&tmp_type, PHALCON_DB_COLUMN_BIND_PARAM_NULL);
						PHALCON_CALL_METHOD(NULL, getThis(), "setbindtype", &value_name, &tmp_type);
					} else if (phalcon_comparestr_str(&value_type, SL("array"), NULL) || phalcon_comparestr_str(&value_type, SL("array-str"), NULL) || phalcon_comparestr_str(&value_type, SL("array-int"), NULL) ) {
						PHALCON_CALL_METHOD(&value_param, getThis(), "getbindparam", &value_name);

						if (Z_TYPE(value_param) != IS_ARRAY) {
							error = 1;
							PHALCON_THROW_EXCEPTION_FORMAT(phalcon_mvc_model_query_exception_ce, "Bind type requires an array in placeholder: %s", Z_STRVAL(value_name));
						} else {
							value_times = phalcon_fast_count_int(&value_param);
							if (value_times < 1) {
								error = 1;
								PHALCON_THROW_EXCEPTION_FORMAT(phalcon_mvc_model_query_exception_ce, "At least one value must be bound in placeholder: %s", Z_STRVAL(value_name));
							} else {
								phalcon_array_update_str_long(return_value, SL("times"), value_times, 0);
							}
						}
						zval_ptr_dtor(&value_param);
					} else {
						error = 1;
						PHALCON_THROW_EXCEPTION_FORMAT(phalcon_mvc_model_query_exception_ce, "Unknown bind type: %s", Z_STRVAL_P(&value_type));
					}
					
					zval_ptr_dtor(&value_parts);
					if (error) {
						zval_ptr_dtor(&left);
						zval_ptr_dtor(&right);
						return;
					}
				} else {
					phalcon_array_fetch_str(&value, expr, SL("value"), PH_NOISY|PH_READONLY);
					PHALCON_CONCAT_SV(&placeholder, ":", &value);

					array_init_size(return_value, 2);
					phalcon_array_update_string_str(return_value, IS(type), SL("placeholder"), 0);
					phalcon_array_update_string(return_value, IS(value), &placeholder, 0);
				}
				break;
			}

			case PHQL_T_NULL:
				array_init_size(return_value, 2);
				phalcon_array_update_string_str(return_value, IS(type), SL("literal"), 0);
				phalcon_array_update_string_str(return_value, IS(value), SL("NULL"), 0);
				break;

			case PHQL_T_LIKE:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("LIKE"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_NLIKE:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("NOT LIKE"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_ILIKE:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("ILIKE"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_NILIKE:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("NOT ILIKE"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_NOT:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("unary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("NOT "), 0);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_ISNULL:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("unary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL(" IS NULL"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				break;

			case PHQL_T_ISNOTNULL:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("unary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL(" IS NOT NULL"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				break;

			case PHQL_T_IN:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("IN"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_NOTIN:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("NOT IN"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_EXISTS:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("unary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("EXISTS"), 0);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_DISTINCT:
				assert(0);
				PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Unexpected PHQL_T_DISTINCT - this should not happen");
				zval_ptr_dtor(&left);
				zval_ptr_dtor(&right);
				return;

			case PHQL_T_BETWEEN:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("BETWEEN"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_AGAINST:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("AGAINST"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_CAST:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("cast"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_CONVERT:
				array_init_size(return_value, 3);
				phalcon_array_update_string_str(return_value, IS(type), SL("convert"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_FCALL:
				PHALCON_RETURN_CALL_METHOD(getThis(), "_getfunctioncall", expr);
				break;

			case PHQL_T_CASE:
				PHALCON_RETURN_CALL_METHOD(getThis(), "_getcaseexpression", expr);
				break;

			case PHQL_T_TS_MATCHES:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("@@"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_TS_OR:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("||"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_TS_AND:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("&&"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_TS_NEGATE:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("!!"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_TS_CONTAINS_ANOTHER:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("@>"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_TS_CONTAINS_IN:
				array_init_size(return_value, 4);
				phalcon_array_update_string_str(return_value, IS(type), SL("binary-op"), 0);
				phalcon_array_update_string_str(return_value, IS(op), SL("<@"), 0);
				phalcon_array_update_string(return_value, IS(left), &left, PH_COPY);
				phalcon_array_update_string(return_value, IS(right), &right, PH_COPY);
				break;

			case PHQL_T_SELECT:
				array_init_size(return_value, 2);

				phalcon_read_property(&models_instances, getThis(), SL("_modelsInstances"), PH_NOISY|PH_READONLY);

				PHALCON_CALL_METHOD(&expr_value, getThis(), "_prepareselect", expr, &PHALCON_GLOBAL(z_true));

				phalcon_update_property(getThis(), SL("_currentModelsInstances"), &models_instances);

				phalcon_array_update_string_str(return_value, IS(type), SL("select"), 0);
				phalcon_array_update_string(return_value, IS(value), &expr_value, 0);
				break;

			default:
				PHALCON_CONCAT_SV(&exception_message, "Unknown expression type ", &expr_type);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
		}
		zval_ptr_dtor(&left);
		zval_ptr_dtor(&right);
		return;
	}

	/**
	 * Is a qualified column
	 */
	if (phalcon_array_isset_str(expr, SL("domain"))) {
		PHALCON_RETURN_CALL_METHOD(getThis(), "_getqualified", expr);
		return;
	}

	/**
	 * Is the expression doesn't have a type it's a list of nodes
	 */
	if (phalcon_array_isset_long(expr, 0)) {
		array_init(&list_items);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(expr), expr_list_item) {
			zval expr_item = {};
			PHALCON_CALL_METHOD(&expr_item, getThis(), "_getexpression", expr_list_item);
			phalcon_array_append(&list_items, &expr_item, 0);
		} ZEND_HASH_FOREACH_END();

		array_init_size(return_value, 2);
		phalcon_array_update_string_str(return_value, IS(type), SL("list"), 0);
		phalcon_array_append(return_value, &list_items, 0);
		return;
	}

	PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Unknown expression");
}

/**
 * Resolves a column from its intermediate representation into an array used to determine
 * if the resulset produced is simple or complex
 *
 * @param array $column
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getSelectColumn){

	zval *column, column_type = {}, models = {}, *model, sql_column = {}, sql_aliases = {}, column_domain = {}, source = {}, phql = {}, exception_message = {};
	zval sql_column_alias = {}, sql_aliases_models = {}, model_name = {}, prepared_alias = {}, column_data = {}, balias = {}, sql_expr_column = {};
	zend_string *str_key;

	phalcon_fetch_params(0, 1, 0, &column);

	if (!phalcon_array_isset_fetch_str(&column_type, column, ISL(type), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 1");
		return;
	}

	/**
	 * Check for select * (all)
	 */
	if (PHALCON_IS_LONG(&column_type, PHQL_T_STARALL)) {
		phalcon_read_property(&models, getThis(), SL("_models"), PH_NOISY|PH_READONLY);

		array_init(return_value);

		ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(models), str_key, model) {
			if (str_key) {
				array_init_size(&sql_column, 3);
				phalcon_array_update_string_str(&sql_column, IS(type), SL("object"), 0);
				phalcon_array_update_string_string(&sql_column, IS(model), str_key, PH_COPY);
				phalcon_array_update_string(&sql_column, IS(column), model, PH_COPY);

				phalcon_array_append(return_value, &sql_column, 0);
			}
		} ZEND_HASH_FOREACH_END();

		return;
	}

	if (!phalcon_array_isset_str(column, SL("column"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 2");
		return;
	}

	/**
	 * Check if selected column is qualified.*
	 */
	if (PHALCON_IS_LONG(&column_type, PHQL_T_DOMAINALL)) {
		phalcon_read_property(&sql_aliases, getThis(), SL("_sqlAliases"), PH_NOISY|PH_READONLY);

		/**
		 * We only allow the alias.*
		 */
		phalcon_array_fetch_str(&column_domain, column, SL("column"), PH_NOISY|PH_READONLY);
		if (!phalcon_array_isset_fetch(&source, &sql_aliases, &column_domain, PH_READONLY)) {
			phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

			PHALCON_CONCAT_SVSV(&exception_message, "Unknown model or alias '", &column_domain, "' (2), when preparing: ", &phql);
			PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
		}

		/**
		 * Get the SQL alias if any
		 */
		ZVAL_COPY_VALUE(&sql_column_alias, &source);

		/**
		 * Get the real source name
		 */
		phalcon_read_property(&sql_aliases_models, getThis(), SL("_sqlAliasesModels"), PH_NOISY|PH_READONLY);

		phalcon_array_fetch(&model_name, &sql_aliases_models, &column_domain, PH_NOISY|PH_READONLY);

		if (PHALCON_IS_EQUAL(&column_domain, &model_name)) {
			phalcon_lcfirst(&prepared_alias, &model_name);
		} else {
			ZVAL_COPY(&prepared_alias, &column_domain);
		}

		/**
		 * The sql_column is a complex type returning a complete object
		 */
		array_init_size(&sql_column, 4);
		phalcon_array_update_string_str(&sql_column, IS(type), SL("object"), 0);
		phalcon_array_update_string(&sql_column, IS(model), &model_name, PH_COPY);
		phalcon_array_update_string(&sql_column, IS(column), &sql_column_alias, PH_COPY);
		phalcon_array_update_string(&sql_column, IS(balias), &prepared_alias, 0);

		array_init_size(return_value, 1);
		phalcon_array_append(return_value, &sql_column, 0);

		return;
	}

	/**
	 * Check for columns qualified and not qualified
	 */
	if (PHALCON_IS_LONG(&column_type, PHQL_T_EXPR)) {
		/**
		 * The sql_column is a scalar type returning a simple string
		 */
		array_init_size(&sql_column, 4);
		phalcon_array_update_string_str(&sql_column, IS(type), SL("scalar"), 0);
		phalcon_array_fetch_str(&column_data, column, SL("column"), PH_NOISY|PH_READONLY);

		PHALCON_CALL_METHOD(&sql_expr_column, getThis(), "_getexpression", &column_data);

		/**
		 * Create balias and sqlAlias
		 */
		if (phalcon_array_isset_fetch_str(&balias, &sql_expr_column, SL("balias"), PH_READONLY)) {
			phalcon_array_update_string(&sql_column, IS(balias), &balias, PH_COPY);
			phalcon_array_update_string(&sql_column, IS(sqlAlias), &balias, PH_COPY);
		}

		phalcon_array_update_string(&sql_column, IS(column), &sql_expr_column, 0);

		array_init_size(return_value, 1);
		phalcon_array_append(return_value, &sql_column, 0);
		return;
	}

	PHALCON_CONCAT_SV(&exception_message, "Unknown type of column ", &column_type);
	PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
}

/**
 * Resolves a table in a SELECT statement checking if the model exists
 *
 * @param Phalcon\Mvc\Model\ManagerInterface $manager
 * @param array $qualifiedName
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getTable){

	zval *manager, *qualified_name, model_name = {}, model = {}, source = {}, schema = {};

	phalcon_fetch_params(0, 2, 0, &manager, &qualified_name);

	if (phalcon_array_isset_fetch_str(&model_name, qualified_name, SL("name"), PH_READONLY)) {
		PHALCON_CALL_METHOD(&model, manager, "load", &model_name);
		PHALCON_CALL_METHOD(&source, &model, "getsource", getThis());
		PHALCON_CALL_METHOD(&schema, &model, "getschema", getThis());
		zval_ptr_dtor(&model);
		if (zend_is_true(&schema)) {
			array_init_size(return_value, 2);
			phalcon_array_append(return_value, &schema, 0);
			phalcon_array_append(return_value, &source, 0);
		} else {
			RETVAL_ZVAL(&source, 0, 0);
		}
		return;
	}
	PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 3");
}

/**
 * Resolves a JOIN clause checking if the associated models exist
 *
 * @param Phalcon\Mvc\Model\ManagerInterface $manager
 * @param array $join
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getJoin){

	zval *manager, *join, qualified = {}, qualified_type = {}, model_name = {}, model = {}, source = {}, schema = {};

	phalcon_fetch_params(0, 2, 0, &manager, &join);

	if (phalcon_array_isset_fetch_str(&qualified, join, SL("qualified"), PH_READONLY)) {
		phalcon_array_fetch_string(&qualified_type, &qualified, IS(type), PH_NOISY|PH_READONLY);
		if (PHALCON_IS_LONG(&qualified_type, PHQL_T_QUALIFIED)) {
			phalcon_array_fetch_string(&model_name, &qualified, IS(name), PH_NOISY|PH_READONLY);

			PHALCON_CALL_METHOD(&model, manager, "load", &model_name);
			PHALCON_CALL_METHOD(&source, &model, "getsource", getThis());
			PHALCON_CALL_METHOD(&schema, &model, "getschema", getThis());

			array_init_size(return_value, 4);
			phalcon_array_update_str(return_value, SL("schema"), &schema, 0);
			phalcon_array_update_str(return_value, SL("source"), &source, 0);
			phalcon_array_update_str(return_value, SL("modelName"), &model_name, PH_COPY);
			phalcon_array_update_str(return_value, SL("model"), &model, 0);
			return;
		}
	}
	PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 4");
}

/**
 * Resolves a JOIN type
 *
 * @param array $join
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getJoinType){

	zval *join, type, phql = {}, exception_message = {};

	phalcon_fetch_params(0, 1, 0, &join);

	if (!phalcon_array_isset_fetch_string(&type, join, IS(type), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 5");
		return;
	}

	switch (phalcon_get_intval(&type)) {

		case PHQL_T_INNERJOIN:
			RETVAL_STRING("INNER");
			break;

		case PHQL_T_LEFTJOIN:
			RETVAL_STRING("LEFT");
			break;

		case PHQL_T_RIGHTJOIN:
			RETVAL_STRING("RIGHT");
			break;

		case PHQL_T_CROSSJOIN:
			RETVAL_STRING("CROSS");
			break;

		case PHQL_T_FULLJOIN:
			RETVAL_STRING("FULL OUTER");
			break;

		default: {
			phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

			PHALCON_CONCAT_SVSV(&exception_message, "Unknown join type ", &type, ", when preparing: ", &phql);
			PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
		}
	}
}

/**
 * Resolves joins involving has-one/belongs-to/has-many relations
 *
 * @param string $joinType
 * @param string $joinSource
 * @param string $modelAlias
 * @param string $joinAlias
 * @param Phalcon\Mvc\Model\RelationInterface $relation
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getSingleJoin){

	zval *join_type, *join_source, *model_alias, *join_alias, *relation, fields = {}, referenced_fields = {}, sql_join_conditions = {};
	zval sql_join_condition = {}, *field, exception_message = {};
	zend_string *str_key;
	ulong idx;

	phalcon_fetch_params(0, 5, 0, &join_type, &join_source, &model_alias, &join_alias, &relation);

	/**
	 * Local fields in the 'from' relation
	 */
	PHALCON_CALL_METHOD(&fields, relation, "getfields");

	/**
	 * Referenced fields in the joined relation
	 */
	PHALCON_CALL_METHOD(&referenced_fields, relation, "getreferencedfields");

	array_init(&sql_join_conditions);

	if (Z_TYPE(fields) != IS_ARRAY) {
		zval left = {}, left_expr = {}, right = {}, right_expr = {};
		/**
		 * Create the left part of the expression
		 */
		array_init_size(&left, 3);
		add_assoc_long_ex(&left, ISL(type), PHQL_T_QUALIFIED);
		phalcon_array_update_string(&left, IS(domain), model_alias, PH_COPY);
		phalcon_array_update_string(&left, IS(name), &fields, PH_COPY);

		PHALCON_CALL_METHOD(&left_expr, getThis(), "_getqualified", &left);
		zval_ptr_dtor(&left);

		/**
		 * Create the right part of the expression
		 */
		array_init_size(&right, 3);
		phalcon_array_update_string_str(&right, IS(type), SL("qualified"), 0);
		phalcon_array_update_string(&right, IS(domain), join_alias, PH_COPY);
		phalcon_array_update_string(&right, IS(name), &referenced_fields, PH_COPY);

		PHALCON_CALL_METHOD(&right_expr, getThis(), "_getqualified", &right);
		zval_ptr_dtor(&right);

		/**
		 * Create a binary operation for the join conditions
		 */
		array_init_size(&sql_join_condition, 4);
		phalcon_array_update_string_str(&sql_join_condition, IS(type), SL("binary-op"), 0);
		phalcon_array_update_string_str(&sql_join_condition, IS(op), SL("="), 0);
		phalcon_array_update_string(&sql_join_condition, IS(left), &left_expr, 0);
		phalcon_array_update_string(&sql_join_condition, IS(right), &right_expr, 0);

		phalcon_array_append(&sql_join_conditions, &sql_join_condition, 0);
	} else {
		/**
		 * Resolve the compound operation
		 */
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(fields), idx, str_key, field) {
			zval tmp = {}, referenced_field = {}, phql = {}, left = {}, left_expr = {}, right = {}, right_expr = {};
			if (str_key) {
				ZVAL_STR(&tmp, str_key);
			} else {
				ZVAL_LONG(&tmp, idx);
			}

			if (!phalcon_array_isset_fetch(&referenced_field, &referenced_fields, &tmp, PH_READONLY)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSVSV(&exception_message, "The number of fields must be equal to the number of referenced fields in join ", model_alias, "-", join_alias, ", when preparing: ", &phql);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}

			/**
			 * Create the left part of the expression
			 */
			array_init_size(&left, 3);
			add_assoc_long_ex(&left, ISL(type), PHQL_T_QUALIFIED);
			phalcon_array_update_string(&left, IS(domain), model_alias, PH_COPY);
			phalcon_array_update_string(&left, IS(name), field, PH_COPY);

			PHALCON_CALL_METHOD(&left_expr, getThis(), "_getqualified", &left);
			zval_ptr_dtor(&left);

			/**
			 * Create the right part of the expression
			 */
			array_init_size(&right, 3);
			phalcon_array_update_string_str(&right, IS(type), SL("qualified"), 0);
			phalcon_array_update_string(&right, IS(domain), join_alias, PH_COPY);
			phalcon_array_update_string(&right, IS(name), &referenced_field, PH_COPY);

			PHALCON_CALL_METHOD(&right_expr, getThis(), "_getqualified", &right);
			zval_ptr_dtor(&right);

			/**
			 * Create a binary operation for the join conditions
			 */
			array_init_size(&sql_join_condition, 4);
			phalcon_array_update_string_str(&sql_join_condition, IS(type), SL("binary-op"), 0);
			phalcon_array_update_string_str(&sql_join_condition, IS(op), SL("="), 0);
			phalcon_array_update_string(&sql_join_condition, IS(left), &left_expr, 0);
			phalcon_array_update_string(&sql_join_condition, IS(right), &right_expr, 0);

			phalcon_array_append(&sql_join_conditions, &sql_join_condition, 0);
		} ZEND_HASH_FOREACH_END();

	}
	zval_ptr_dtor(&fields);
	zval_ptr_dtor(&referenced_fields);

	/**
	 * A single join
	 */
	array_init_size(return_value, 3);
	phalcon_array_update_string(return_value, IS(type), join_type, PH_COPY);
	phalcon_array_update_string(return_value, IS(source), join_source, PH_COPY);
	phalcon_array_update_string(return_value, IS(conditions), &sql_join_conditions, 0);
}

/**
 * Resolves joins involving many-to-many relations
 *
 * @param string $joinType
 * @param string $joinSource
 * @param string $modelAlias
 * @param string $joinAlias
 * @param Phalcon\Mvc\Model\RelationInterface $relation
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getMultiJoin){

	zval *join_type, *join_source, *model_alias, *join_alias, *relation, fields = {}, referenced_fields = {}, intermediate_model_name = {}, manager = {};
	zval intermediate_model = {}, intermediate_source = {}, intermediate_fields = {}, intermediate_referenced_fields = {};
	zval referenced_model_name = {}, *field, left = {}, left_expr = {}, right = {}, right_expr = {}, exception_message = {};
	zval sql_join_condition_first = {}, sql_join_conditions_first = {}, sql_join_first = {}, sql_join_condition_second = {}, sql_join_conditions_second = {}, sql_join_second = {};
	zend_string *str_key;
	ulong idx;

	phalcon_fetch_params(0, 5, 0, &join_type, &join_source, &model_alias, &join_alias, &relation);

	array_init(return_value);

	/**
	 * Local fields in the 'from' relation
	 */
	PHALCON_CALL_METHOD(&fields, relation, "getfields");

	/**
	 * Referenced fields in the joined relation
	 */
	PHALCON_CALL_METHOD(&referenced_fields, relation, "getreferencedfields");

	/**
	 * Intermediate model
	 */
	PHALCON_CALL_METHOD(&intermediate_model_name, relation, "getintermediatemodel");

	PHALCON_CALL_SELF(&manager, "getmodelsmanager");

	/**
	 * Get the intermediate model instance
	 */
	PHALCON_CALL_METHOD(&intermediate_model, &manager, "load", &intermediate_model_name);
	zval_ptr_dtor(&manager);

	/**
	 * Source of the related model
	 */
	PHALCON_CALL_METHOD(&intermediate_source, &intermediate_model, "getsource", getThis());

	/**
	 * Update the internal sqlAliases to set up the intermediate model
	 */
	phalcon_update_property_array(getThis(), SL("_sqlAliases"), &intermediate_model_name, &intermediate_source);

	/**
	 * Update the internal _sqlAliasesModelsInstances to rename columns if necessary
	 */
	phalcon_update_property_array(getThis(), SL("_sqlAliasesModelsInstances"), &intermediate_model_name, &intermediate_model);
	zval_ptr_dtor(&intermediate_model);

	/**
	 * Fields that join the 'from' model with the 'intermediate' model
	 */
	PHALCON_CALL_METHOD(&intermediate_fields, relation, "getintermediatefields");

	/**
	 * Fields that join the 'intermediate' model with the intermediate model
	 */
	PHALCON_CALL_METHOD(&intermediate_referenced_fields, relation, "getintermediatereferencedfields");

	/**
	 * Intermediate model
	 */
	PHALCON_CALL_METHOD(&referenced_model_name, relation, "getreferencedmodel");
	if (Z_TYPE(fields) == IS_ARRAY) {
		/** @todo The code seems dead - the empty array will be returned */
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(fields), idx, str_key, field) {
			zval tmp = {}, phql = {}, sql_equals_join_condition = {};
			if (str_key) {
				ZVAL_STR(&tmp, str_key);
			} else {
				ZVAL_LONG(&tmp, idx);
			}

			if (!phalcon_array_isset(&referenced_fields, &tmp)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSVSV(&exception_message, "The number of fields must be equal to the number of referenced fields in join ", model_alias, "-", join_alias, ", when preparing: ", &phql);
				PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}

			/**
			 * Create the left part of the expression
			 */
			array_init_size(&left, 3);
			add_assoc_long_ex(&left, ISL(type), PHQL_T_QUALIFIED);
			phalcon_array_update_string(&left, IS(domain), model_alias, PH_COPY);
			phalcon_array_update_string(&left, IS(name), field, PH_COPY);

			PHALCON_CALL_METHOD(&left_expr, getThis(), "_getqualified", &left);
			zval_ptr_dtor(&left);

			/**
			 * Create the right part of the expression
			 */
			array_init_size(&right, 3);
			phalcon_array_update_string_str(&right, IS(type), SL("qualified"), 0);
			phalcon_array_update_string(&right, IS(domain), join_alias, PH_COPY);
			phalcon_array_update_string(&right, IS(name), &referenced_fields, PH_COPY);

			PHALCON_CALL_METHOD(&right_expr, getThis(), "_getqualified", &right);
			zval_ptr_dtor(&right);

			/**
			 * Create a binary operation for the join conditions
			 */
			array_init_size(&sql_equals_join_condition, 4);
			phalcon_array_update_string_str(&sql_equals_join_condition, IS(type), SL("binary-op"), 0);
			phalcon_array_update_string_str(&sql_equals_join_condition, IS(op), SL("="), 0);
			phalcon_array_update_string(&sql_equals_join_condition, IS(left), &left_expr, 0);
			phalcon_array_update_string(&sql_equals_join_condition, IS(right), &right_expr, 0);
		} ZEND_HASH_FOREACH_END();
	} else {
		/**
		 * Create the left part of the expression
		 */
		array_init_size(&left, 3);
		add_assoc_long_ex(&left, ISL(type), PHQL_T_QUALIFIED);
		phalcon_array_update_string(&left, IS(domain), model_alias, PH_COPY);
		phalcon_array_update_string(&left, IS(name), &fields, PH_COPY);

		PHALCON_CALL_METHOD(&left_expr, getThis(), "_getqualified", &left);
		zval_ptr_dtor(&left);

		/**
		 * Create the right part of the expression
		 */
		array_init_size(&right, 3);
		phalcon_array_update_string_str(&right, IS(type), SL("qualified"), 0);
		phalcon_array_update_string(&right, IS(domain), &intermediate_model_name, PH_COPY);
		phalcon_array_update_string(&right, IS(name), &intermediate_fields, PH_COPY);

		PHALCON_CALL_METHOD(&right_expr, getThis(), "_getqualified", &right);
		zval_ptr_dtor(&right);

		/**
		 * Create a binary operation for the join conditions
		 */
		array_init_size(&sql_join_condition_first, 4);
		phalcon_array_update_string_str(&sql_join_condition_first, IS(type), SL("binary-op"), 0);
		phalcon_array_update_string_str(&sql_join_condition_first, IS(op), SL("="), 0);
		phalcon_array_update_string(&sql_join_condition_first, IS(left), &left_expr, 0);
		phalcon_array_update_string(&sql_join_condition_first, IS(right), &right_expr, 0);

		array_init_size(&sql_join_conditions_first, 1);
		phalcon_array_append(&sql_join_conditions_first, &sql_join_condition_first, 0);

		/**
		 * A single join
		 */
		array_init_size(&sql_join_first, 3);
		phalcon_array_update_string(&sql_join_first, IS(type), join_type, PH_COPY);
		phalcon_array_update_string(&sql_join_first, IS(source), &intermediate_source, PH_COPY);
		phalcon_array_update_string(&sql_join_first, IS(conditions), &sql_join_conditions_first, 0);

		/**
		 * Create the left part of the expression
		 */
		array_init_size(&left, 3);
		add_assoc_long_ex(&left, ISL(type), PHQL_T_QUALIFIED);
		phalcon_array_update_string(&left, IS(domain), &intermediate_model_name, PH_COPY);
		phalcon_array_update_string(&left, IS(name), &intermediate_referenced_fields, PH_COPY);

		PHALCON_CALL_METHOD(&left_expr, getThis(), "_getqualified", &left);
		zval_ptr_dtor(&left);

		/**
		 * Create the right part of the expression
		 */
		array_init_size(&right, 3);
		phalcon_array_update_string_str(&right, IS(type), SL("qualified"), 0);
		phalcon_array_update_string(&right, IS(domain), &referenced_model_name, PH_COPY);
		phalcon_array_update_string(&right, IS(name), &referenced_fields, PH_COPY);

		PHALCON_CALL_METHOD(&right_expr, getThis(), "_getqualified", &right);
		zval_ptr_dtor(&right);

		/**
		 * Create a binary operation for the join conditions
		 */
		array_init_size(&sql_join_condition_second, 4);
		phalcon_array_update_string_str(&sql_join_condition_second, IS(type), SL("binary-op"), 0);
		phalcon_array_update_string_str(&sql_join_condition_second, IS(op), SL("="), 0);
		phalcon_array_update_string(&sql_join_condition_second, IS(left), &left_expr, 0);
		phalcon_array_update_string(&sql_join_condition_second, IS(right), &right_expr, 0);

		array_init_size(&sql_join_conditions_second, 1);
		phalcon_array_append(&sql_join_conditions_second, &sql_join_condition_second, 0);

		/**
		 * A single join
		 */
		array_init_size(&sql_join_second, 3);
		phalcon_array_update_string(&sql_join_second, IS(type), join_type, PH_COPY);
		phalcon_array_update_string(&sql_join_second, IS(source), join_source, PH_COPY);
		phalcon_array_update_string(&sql_join_second, IS(conditions), &sql_join_conditions_second, 0);

		phalcon_array_update_long(return_value, 0, &sql_join_first, 0);
		phalcon_array_update_long(return_value, 1, &sql_join_second, 0);
	}
	zval_ptr_dtor(&fields);
	zval_ptr_dtor(&referenced_fields);
	zval_ptr_dtor(&referenced_model_name);
	zval_ptr_dtor(&intermediate_model_name);
	zval_ptr_dtor(&intermediate_referenced_fields);
	zval_ptr_dtor(&intermediate_source);
}

/**
 * Processes the JOINs in the query returning an internal representation for the database dialect
 *
 * @param array $select
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getJoins){
	zval *select, models = {}, sql_aliases = {}, sql_aliases_models = {}, sql_models_aliases = {}, sql_aliases_models_instances = {}, models_instances = {};
	zval from_models = {}, *form_model, sql_joins = {}, join_models = {}, join_sources = {}, join_types = {}, join_pre_condition = {}, join_prepared = {};
	zval manager = {}, joins = {}, select_joins = {}, *join_item;
	zend_string *str_key, *str_key2;
	ulong idx, idx2;

	phalcon_fetch_params(1, 1, 0, &select);

	phalcon_read_property(&models, getThis(), SL("_models"), PH_NOISY|PH_READONLY);
	phalcon_read_property(&sql_aliases, getThis(), SL("_sqlAliases"), PH_NOISY|PH_READONLY);
	phalcon_read_property(&sql_aliases_models, getThis(), SL("_sqlAliasesModels"), PH_NOISY|PH_READONLY);
	phalcon_read_property(&sql_models_aliases, getThis(), SL("_sqlModelsAliases"), PH_NOISY|PH_READONLY);
	phalcon_read_property(&sql_aliases_models_instances, getThis(), SL("_sqlAliasesModelsInstances"), PH_NOISY|PH_READONLY);
	phalcon_read_property(&models_instances, getThis(), SL("_modelsInstances"), PH_NOISY|PH_READONLY);

	PHALCON_MM_ZVAL_DUP(&from_models, &models);

	array_init(&sql_joins);
	PHALCON_MM_ADD_ENTRY(&sql_joins);
	array_init(&join_models);
	PHALCON_MM_ADD_ENTRY(&join_models);
	array_init(&join_sources);
	PHALCON_MM_ADD_ENTRY(&join_sources);
	array_init(&join_types);
	PHALCON_MM_ADD_ENTRY(&join_types);
	array_init(&join_pre_condition);
	PHALCON_MM_ADD_ENTRY(&join_pre_condition);
	array_init(&join_prepared);
	PHALCON_MM_ADD_ENTRY(&join_prepared);

	PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
	PHALCON_MM_ADD_ENTRY(&manager);

	phalcon_array_fetch_str(&joins, select, SL("joins"), PH_NOISY|PH_READONLY);
	if (!phalcon_array_isset_long(&joins, 0)) {
		array_init_size(&select_joins, 1);
		phalcon_array_append(&select_joins, &joins, PH_COPY);
	} else {
		PHALCON_ZVAL_DUP(&select_joins, &joins);
	}
	PHALCON_MM_ADD_ENTRY(&select_joins);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(select_joins), join_item) {
		zval join_data = {}, source = {}, schema = {}, model = {}, model_name = {}, complete_source = {}, join_type = {}, alias_expr = {}, alias = {}, phql = {}, exception_message = {};
		/**
		 * Check join alias
		 */
		PHALCON_MM_CALL_METHOD(&join_data, getThis(), "_getjoin", &manager, join_item);
		PHALCON_MM_ADD_ENTRY(&join_data);

		phalcon_array_fetch_str(&source, &join_data, SL("source"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_str(&schema, &join_data, SL("schema"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_str(&model, &join_data, SL("model"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_str(&model_name, &join_data, SL("modelName"), PH_NOISY|PH_READONLY);

		array_init_size(&complete_source, 2);
		phalcon_array_append(&complete_source, &source, PH_COPY);
		phalcon_array_append(&complete_source, &schema, PH_COPY);
		PHALCON_MM_ADD_ENTRY(&complete_source);

		/**
		 * Check join alias
		 */
		PHALCON_MM_CALL_METHOD(&join_type, getThis(), "_getjointype", join_item);
		PHALCON_MM_ADD_ENTRY(&join_type);

		/**
		 * Process join alias
		 */
		if (phalcon_array_isset_fetch_str(&alias_expr, join_item, SL("alias"), PH_READONLY)) {
			phalcon_array_fetch_string(&alias, &alias_expr, IS(name), PH_NOISY|PH_READONLY);
			/**
			 * Check if alias is unique
			 */
			if (phalcon_array_isset(&join_models, &alias)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSV(&exception_message, "Cannot use '", &alias, "' as join alias because it was already used when preparing: ", &phql);
				PHALCON_MM_ADD_ENTRY(&exception_message);
				PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}

			/**
			 * Add the alias to the source
			 */
			phalcon_array_append(&complete_source, &alias, PH_COPY);

			/**
			 * Set the join type
			 */
			phalcon_array_update(&join_types, &alias, &join_type, PH_COPY);

			/**
			 * Update alias => alias
			 */
			phalcon_array_update(&sql_aliases, &alias, &alias, PH_COPY);

			/**
			 * Update model => alias
			 */
			phalcon_array_update(&join_models, &alias, &model_name, PH_COPY);

			/**
			 * Update model => alias
			 */
			phalcon_array_update(&sql_models_aliases, &model_name, &alias, PH_COPY);

			/**
			 * Update alias => model
			 */
			phalcon_array_update(&sql_aliases_models, &alias, &model_name, PH_COPY);

			/**
			 * Update alias => model
			 */
			phalcon_array_update(&sql_aliases_models_instances, &alias, &model, PH_COPY);

			/**
			 * Update model => alias
			 */
			phalcon_array_update(&models, &model_name, &alias, PH_COPY);

			/**
			 * Complete source related to a model
			 */
			phalcon_array_update(&join_sources, &alias, &complete_source, PH_COPY);

			/**
			 * Complete source related to a model
			 */
			phalcon_array_update(&join_prepared, &alias, join_item, PH_COPY);
		} else {
			/**
			 * Check if alias is unique
			 */
			if (phalcon_array_isset(&join_models, &model_name)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSV(&exception_message, "Cannot use '", &model_name, "' as join alias because it was already used when preparing: ", &phql);
				PHALCON_MM_ADD_ENTRY(&exception_message);
				PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}

			/**
			 * Set the join type
			 */
			phalcon_array_update(&join_types, &model_name, &join_type, PH_COPY);

			/**
			 * Update model => source
			 */
			phalcon_array_update(&sql_aliases, &model_name, &source, PH_COPY);

			/**
			 * Update model => source
			 */
			phalcon_array_update(&join_models, &model_name, &source, PH_COPY);

			/**
			 * Update model => model
			 */
			phalcon_array_update(&sql_models_aliases, &model_name, &model_name, PH_COPY);

			/**
			 * Update model => model
			 */
			phalcon_array_update(&sql_aliases_models, &model_name, &model_name, PH_COPY);

			/**
			 * Update model => model instance
			 */
			phalcon_array_update(&sql_aliases_models_instances, &model_name, &model, PH_COPY);

			/**
			 * Update model => source
			 */
			phalcon_array_update(&models, &model_name, &source, PH_COPY);

			/**
			 * Complete source related to a model
			 */
			phalcon_array_update(&join_sources, &model_name, &complete_source, PH_COPY);

			/**
			 * Complete source related to a model
			 */
			phalcon_array_update(&join_prepared, &model_name, join_item, PH_COPY);
		}

		phalcon_array_update(&models_instances, &model_name, &model, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	/**
	 * Update temporary properties
	 */
	phalcon_update_property(getThis(), SL("_currentModelsInstances"), &models_instances);

	phalcon_update_property(getThis(), SL("_models"), &models);
	phalcon_update_property(getThis(), SL("_sqlAliases"), &sql_aliases);
	phalcon_update_property(getThis(), SL("_sqlAliasesModels"), &sql_aliases_models);
	phalcon_update_property(getThis(), SL("_sqlModelsAliases"), &sql_models_aliases);
	phalcon_update_property(getThis(), SL("_sqlAliasesModelsInstances"), &sql_aliases_models_instances);
	phalcon_update_property(getThis(), SL("_modelsInstances"), &models_instances);

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(join_prepared), idx, str_key, join_item) {
		zval tmp = {}, join_expr = {}, pre_condition = {};
		if (str_key) {
			ZVAL_STR(&tmp, str_key);
		} else {
			ZVAL_LONG(&tmp, idx);
		}

		/**
		 * Check for predefined conditions
		 */
		if (phalcon_array_isset_fetch_str(&join_expr, join_item, SL("conditions"), PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&pre_condition, getThis(), "_getexpression", &join_expr);
			PHALCON_MM_ADD_ENTRY(&pre_condition);
			phalcon_array_update(&join_pre_condition, &tmp, &pre_condition, PH_COPY);
		}
	} ZEND_HASH_FOREACH_END();

	/**
	 * Create join relationships dynamically
	 */
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(&from_models), idx, str_key, form_model) {
		zval tmp = {}, *join_model;
		if (str_key) {
			ZVAL_STR(&tmp, str_key);
		} else {
			ZVAL_LONG(&tmp, idx);
		}

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(join_models), idx2, str_key2, join_model) {
			zval tmp2 = {}, join_source = {}, join_type = {}, model_name_alias = {}, relation = {}, relations = {}, model_alias = {}, sql_join = {};
			zval sql_join_conditions = {}, pre_condition = {}, is_through = {}, phql = {}, exception_message = {};
			if (str_key2) {
				ZVAL_STR(&tmp2, str_key2);
			} else {
				ZVAL_LONG(&tmp2, idx2);
			}

			/**
			 * Real source name for joined model
			 */
			phalcon_array_fetch(&join_source, &join_sources, &tmp2, PH_NOISY|PH_READONLY);

			/**
			 * Join type is: LEFT, RIGHT, INNER, etc
			 */
			phalcon_array_fetch(&join_type, &join_types, &tmp2, PH_NOISY|PH_READONLY);

			/**
			 * Check if the model already have pre-defined conditions
			 */
			if (!phalcon_array_isset(&join_pre_condition, &tmp2)) {

				/**
				 * Get the model name from its source
				 */
				phalcon_array_fetch(&model_name_alias, &sql_aliases_models, &tmp2, PH_NOISY|PH_READONLY);

				/**
				 * Check if the joined model is an alias
				 */
				PHALCON_MM_CALL_METHOD(&relation, &manager, "getrelationbyalias", &tmp, &model_name_alias);
				PHALCON_MM_ADD_ENTRY(&relation);
				if (PHALCON_IS_FALSE(&relation)) {
					/**
					 * Check for relations between models
					 */
					PHALCON_MM_CALL_METHOD(&relations, &manager, "getrelationsbetween", &tmp, &model_name_alias);
					PHALCON_MM_ADD_ENTRY(&relations);
					if (Z_TYPE(relations) == IS_ARRAY) {
						/**
						 * More than one relation must throw an exception
						 */
						if (zend_hash_num_elements(Z_ARRVAL(relations)) != 1) {
							phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

							PHALCON_CONCAT_SVSVSV(&exception_message, "There is more than one relation between models '", form_model, "' and '", join_model, "\", the join must be done using an alias when preparing: ", &phql);
							PHALCON_MM_ADD_ENTRY(&exception_message);
							PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
							return;
						}

						/**
						 * Get the first relationship
						 */
						phalcon_array_fetch_long(&relation, &relations, 0, PH_NOISY|PH_READONLY);
					}
				}

				/**
				 * Valid relations are objects
				 */
				if (Z_TYPE(relation) == IS_OBJECT) {
					/**
					 * Get the related model alias of the left part
					 */
					phalcon_array_fetch(&model_alias, &sql_models_aliases, &tmp, PH_NOISY|PH_READONLY);

					/**
					 * Generate the conditions based on the type of join
					 */
					PHALCON_MM_CALL_METHOD(&is_through, &relation, "isthrough");
					if (!zend_is_true(&is_through)) {
						PHALCON_MM_CALL_METHOD(&sql_join, getThis(), "_getsinglejoin", &join_type, &join_source, &model_alias, &tmp2, &relation);
					} else {
						PHALCON_MM_CALL_METHOD(&sql_join, getThis(), "_getmultijoin", &join_type, &join_source, &model_alias, &tmp2, &relation);
					}

					/**
					 * Append or merge joins
					 */
					if (phalcon_array_isset_long(&sql_join, 0)) {
						zval new_sql_joins = {};
						phalcon_fast_array_merge(&new_sql_joins, &sql_joins, &sql_join);
						zval_ptr_dtor(&sql_join);
						PHALCON_MM_ADD_ENTRY(&new_sql_joins);
						ZVAL_COPY_VALUE(&sql_joins, &new_sql_joins);
					} else {
						phalcon_array_append(&sql_joins, &sql_join, 0);
					}
				} else {
					array_init(&sql_join_conditions);

					/**
					 * Join without conditions because no relation has been found between the models
					 */
					array_init_size(&sql_join, 3);
					phalcon_array_update_string(&sql_join, IS(type), &join_type, PH_COPY);
					phalcon_array_update_string(&sql_join, IS(source), &join_source, PH_COPY);
					phalcon_array_update_string(&sql_join, IS(conditions), &sql_join_conditions, 0);

					phalcon_array_append(&sql_joins, &sql_join, 0);
				}
			} else {
				/**
				 * Get the conditions stablished by the developer
				 */
				phalcon_array_fetch(&pre_condition, &join_pre_condition, &tmp2, PH_NOISY|PH_READONLY);
				array_init_size(&sql_join_conditions, 1);
				phalcon_array_append(&sql_join_conditions, &pre_condition, PH_COPY);

				/**
				 * Join with conditions stablished by the developer
				 */
				array_init_size(&sql_join, 3);
				phalcon_array_update_string(&sql_join, IS(type), &join_type, PH_COPY);
				phalcon_array_update_string(&sql_join, IS(source), &join_source, PH_COPY);
				phalcon_array_update_string(&sql_join, IS(conditions), &sql_join_conditions, 0);
				phalcon_array_append(&sql_joins, &sql_join, 0);
			}
		} ZEND_HASH_FOREACH_END();
	} ZEND_HASH_FOREACH_END();

	RETURN_MM_CTOR(&sql_joins);
}

/**
 * Returns a processed order clause for a SELECT statement
 *
 * @param array $order
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getOrderClause){

	zval *order, order_columns = {}, *order_item;

	phalcon_fetch_params(1, 1, 0, &order);

	if (!phalcon_array_isset_long(order, 0)) {
		array_init_size(&order_columns, 1);
		PHALCON_MM_ADD_ENTRY(&order_columns);
		phalcon_array_append(&order_columns, order, PH_COPY);
	} else {
		ZVAL_COPY_VALUE(&order_columns, order);
	}

	array_init(return_value);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(order_columns), order_item) {
		zval order_column = {}, order_sort = {}, order_part_expr = {}, order_part_sort = {};

		array_init(&order_part_sort);
		PHALCON_MM_ADD_ENTRY(&order_part_sort);
		phalcon_array_fetch_str(&order_column, order_item, SL("column"), PH_NOISY|PH_READONLY);

		PHALCON_MM_CALL_METHOD(&order_part_expr, getThis(), "_getexpression", &order_column);

		/**
		* Check if the order has a predefined ordering mode
		*/
		if (phalcon_array_isset_fetch_str(&order_sort, order_item, SL("sort"), PH_READONLY)) {
			phalcon_array_append(&order_part_sort, &order_part_expr, 0);
			if (PHALCON_IS_LONG(&order_sort, PHQL_T_ASC)) {
				add_next_index_stringl(&order_part_sort, SL("ASC"));
			} else {
				add_next_index_stringl(&order_part_sort, SL("DESC"));
			}
		} else {
			phalcon_array_append(&order_part_sort, &order_part_expr, 0);
		}

		phalcon_array_append(return_value, &order_part_sort, PH_COPY);
	} ZEND_HASH_FOREACH_END();
	RETURN_MM();
}

/**
 * Returns a processed group clause for a SELECT statement
 *
 * @param array $group
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _getGroupClause){

	zval *group, *group_item, group_part_expr = {};

	phalcon_fetch_params(0, 1, 0, &group);

	if (phalcon_array_isset_long(group, 0)) {

		/**
		 * The select is grouped by several columns
		 */
		array_init(return_value);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(group), group_item) {
			PHALCON_CALL_METHOD(&group_part_expr, getThis(), "_getexpression", group_item);
			phalcon_array_append(return_value, &group_part_expr, 0);
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_CALL_METHOD(&group_part_expr, getThis(), "_getexpression", group);

		array_init_size(return_value, 1);
		phalcon_array_append(return_value, &group_part_expr, 0);
	}
}

PHP_METHOD(Phalcon_Mvc_Model_Query, _getLimitClause) {

	zval *limit_clause, tmp = {}, limit = {}, offset = {};

	phalcon_fetch_params(0, 1, 0, &limit_clause);
	assert(Z_TYPE_P(limit_clause) == IS_ARRAY);

	array_init(return_value);

	if (likely(phalcon_array_isset_fetch_str(&limit, limit_clause, SL("number"), PH_READONLY))) {
		PHALCON_CALL_METHOD(&tmp, getThis(), "_getexpression", &limit);
		phalcon_array_update_string(return_value, IS(number), &tmp, 0);
	}

	if (phalcon_array_isset_fetch_str(&offset, limit_clause, SL("offset"), PH_READONLY)) {
		PHALCON_CALL_METHOD(&tmp, getThis(), "_getexpression", &offset);
		phalcon_array_update_string(return_value, IS(offset), &tmp, 0);
	}
}

/**
 * Analyzes a SELECT intermediate code and produces an array to be executed later
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareSelect){

	zval *_ast = NULL, *merge = NULL, event_name = {}, old_ast = {}, ast = {}, select = {}, tables = {}, columns = {}, distinct = {}, sql_models = {}, sql_tables = {};
	zval sql_aliases = {}, sql_columns = {}, sql_aliases_models = {}, sql_models_aliases = {}, sql_aliases_models_instances = {}, models = {};
	zval models_instances = {}, selected_models = {}, manager = {}, *selected_model, tmp_models = {}, tmp_models_instances = {}, tmp_sql_aliases = {};
	zval tmp_sql_aliases_models = {}, tmp_sql_models_aliases = {}, tmp_sql_aliases_models_instances = {}, joins = {}, sql_joins = {}, select_columns = {};
	zval position = {}, sql_column_aliases = {}, *column, sql_select = {}, where = {}, where_expr = {}, group_by = {}, sql_group = {};
	zval having = {}, having_expr = {}, order = {}, sql_order = {}, limit = {}, sql_limit = {}, forupdate = {};

	phalcon_fetch_params(1, 0, 2, &_ast, &merge);

	if (!merge) {
		merge = &PHALCON_GLOBAL(z_false);
	}

	if (_ast) {
		phalcon_read_property(&old_ast, getThis(), SL("_ast"), PH_COPY);
		PHALCON_MM_ADD_ENTRY(&old_ast);
		phalcon_update_property(getThis(), SL("_ast"), _ast);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforePrepareSelect");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	phalcon_read_property(&ast, getThis(), SL("_ast"), PH_NOISY|PH_READONLY);

	if (!phalcon_array_isset_fetch_str(&select, &ast, SL("select"), PH_READONLY)) {
		ZVAL_COPY_VALUE(&select, &ast);
	}

	if (!phalcon_array_isset_fetch_str(&tables, &select, SL("tables"), PH_READONLY)) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 6");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&columns, &select, SL("columns"), PH_READONLY)) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted SELECT AST 7");
		return;
	}

	/**
	 * sql_models are all the models that are using in the query
	 */
	array_init(&sql_models);
	PHALCON_MM_ADD_ENTRY(&sql_models);

	/**
	 * sql_tables are all the mapped sources regarding the models in use
	 */
	array_init(&sql_tables);
	PHALCON_MM_ADD_ENTRY(&sql_tables);

	/**
	 * sql_aliases are the aliases as keys and the mapped sources as values
	 */
	array_init(&sql_aliases);
	PHALCON_MM_ADD_ENTRY(&sql_aliases);

	/**
	 * sql_columns are all every column expression
	 */
	array_init(&sql_columns);
	PHALCON_MM_ADD_ENTRY(&sql_columns);

	/**
	 * sql_aliases_models are the aliases as keys and the model names as values
	 */
	array_init(&sql_aliases_models);
	PHALCON_MM_ADD_ENTRY(&sql_aliases_models);

	/**
	 * sql_aliases_models are the model names as keys and the aliases as values
	 */
	array_init(&sql_models_aliases);
	PHALCON_MM_ADD_ENTRY(&sql_models_aliases);

	/**
	 * sql_aliases_models_instances are the aliases as keys and the model instances as
	 * values
	 */
	array_init(&sql_aliases_models_instances);
	PHALCON_MM_ADD_ENTRY(&sql_aliases_models_instances);

	/**
	 * Models information
	 */
	array_init(&models);
	PHALCON_MM_ADD_ENTRY(&models);
	array_init(&models_instances);
	PHALCON_MM_ADD_ENTRY(&models_instances);

	if (!phalcon_array_isset_long(&tables, 0)) {
		array_init_size(&selected_models, 1);
		PHALCON_MM_ADD_ENTRY(&selected_models);
		phalcon_array_append(&selected_models, &tables, PH_COPY);
	} else {
		ZVAL_COPY_VALUE(&selected_models, &tables);
	}

	PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
	PHALCON_MM_ADD_ENTRY(&manager);

	/**
	 * Processing selected columns
	 */
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(selected_models), selected_model) {
		zval qualified_name = {}, model_name = {}, ns_alias = {}, real_namespace = {}, real_model_name = {};
		zval model = {}, schema = {}, source = {}, indexs = {}, complete_source = {}, alias = {}, phql = {}, exception_message = {};

		phalcon_array_fetch_str(&qualified_name, selected_model, SL("qualifiedName"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_string(&model_name, &qualified_name, IS(name), PH_NOISY|PH_READONLY);

		/**
		 * Check if the table have a namespace alias
		 */
		if (phalcon_memnstr_str(&model_name, SL(":"))) {
			zval parts = {}, name = {};
			phalcon_fast_explode_str(&parts, SL(":"), &model_name);
			PHALCON_MM_ADD_ENTRY(&parts);
			phalcon_array_fetch_long(&ns_alias, &parts, 0, PH_NOISY|PH_READONLY);
			phalcon_array_fetch_long(&name, &parts, 1, PH_NOISY|PH_READONLY);

			/**
			 * Get the real namespace alias
			 */
			PHALCON_MM_CALL_METHOD(&real_namespace, &manager, "getnamespacealias", &ns_alias);
			PHALCON_MM_ADD_ENTRY(&real_namespace);

			/**
			 * Create the real namespaced name
			 */
			PHALCON_CONCAT_VSV(&real_model_name, &real_namespace, "\\", &name);
			PHALCON_MM_ADD_ENTRY(&real_model_name);
		} else {
			ZVAL_COPY_VALUE(&real_model_name, &model_name);
		}

		/**
		 * Load a model instance from the models manager
		 */
		PHALCON_MM_CALL_METHOD(&model, &manager, "load", &real_model_name);
		PHALCON_MM_ADD_ENTRY(&model);

		/**
		 * Define a complete schema/source
		 */
		PHALCON_MM_CALL_METHOD(&schema, &model, "getschema", getThis());
		PHALCON_MM_ADD_ENTRY(&schema);
		PHALCON_MM_CALL_METHOD(&source, &model, "getsource", getThis());
		PHALCON_MM_ADD_ENTRY(&source);

		/**
		 * Obtain the real source including the schema
		 */
		if (zend_is_true(&schema)) {
			array_init_size(&complete_source, 2);
			PHALCON_MM_ADD_ENTRY(&complete_source);
			phalcon_array_append(&complete_source, &source, PH_COPY);
			phalcon_array_append(&complete_source, &schema, PH_COPY);
		} else {
			ZVAL_COPY_VALUE(&complete_source, &source);
		}

		/**
		 * If an alias is defined for a model the model cannot be referenced in the column
		 * list
		 */
		if (phalcon_array_isset_fetch_str(&alias, selected_model, SL("alias"), PH_READONLY)) {
			/**
			 * Check that the alias hasn't been used before
			 */
			if (phalcon_array_isset(&sql_aliases, &alias)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSV(&exception_message, "Alias \"", &alias, " is already used when preparing: ", &phql);
				PHALCON_MM_ADD_ENTRY(&exception_message);
				PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}

			phalcon_array_update(&sql_aliases, &alias, &alias, PH_COPY);
			phalcon_array_update(&sql_aliases_models, &alias, &model_name, PH_COPY);
			phalcon_array_update(&sql_models_aliases, &model_name, &alias, PH_COPY);
			phalcon_array_update(&sql_aliases_models_instances, &alias, &model, PH_COPY);

			/**
			 * Append or convert complete source to an array
			 */
			if (Z_TYPE(complete_source) == IS_ARRAY) {
				phalcon_array_append(&complete_source, &alias, PH_COPY);
			} else {
				array_init_size(&complete_source, 3);
				PHALCON_MM_ADD_ENTRY(&complete_source);
				phalcon_array_append(&complete_source, &source, PH_COPY);
				add_next_index_null(&complete_source);
				phalcon_array_append(&complete_source, &alias, PH_COPY);
			}

			phalcon_array_update(&models, &model_name, &alias, PH_COPY);
		} else {
			phalcon_array_update(&sql_aliases, &model_name, &source, PH_COPY);
			phalcon_array_update(&sql_aliases_models, &model_name, &model_name, PH_COPY);
			phalcon_array_update(&sql_models_aliases, &model_name, &model_name, PH_COPY);
			phalcon_array_update(&sql_aliases_models_instances, &model_name, &model, PH_COPY);
			phalcon_array_update(&models, &model_name, &source, PH_COPY);
		}

		if (phalcon_array_isset_fetch_str(&indexs, selected_model, SL("indexs"), PH_READONLY)) {
			phalcon_array_append(&complete_source, &indexs, PH_COPY);
		}

		phalcon_array_append(&sql_models, &model_name, PH_COPY);
		phalcon_array_append(&sql_tables, &complete_source, PH_COPY);
		phalcon_array_update(&models_instances, &model_name, &model, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	/**
	 * Assign Models/Tables information
	 */
	phalcon_update_property(getThis(), SL("_currentModelsInstances"), &models_instances);

	if (!zend_is_true(merge)) {
		phalcon_update_property(getThis(), SL("_models"), &models);
		phalcon_update_property(getThis(), SL("_modelsInstances"), &models_instances);
		phalcon_update_property(getThis(), SL("_sqlAliases"), &sql_aliases);
		phalcon_update_property(getThis(), SL("_sqlAliasesModels"), &sql_aliases_models);
		phalcon_update_property(getThis(), SL("_sqlModelsAliases"), &sql_models_aliases);
		phalcon_update_property(getThis(), SL("_sqlAliasesModelsInstances"), &sql_aliases_models_instances);
	} else {
		phalcon_read_property(&tmp_models, getThis(), SL("_models"), PH_NOISY|PH_READONLY);
		phalcon_read_property(&tmp_models_instances, getThis(), SL("_modelsInstances"), PH_NOISY|PH_READONLY);
		phalcon_read_property(&tmp_sql_aliases, getThis(), SL("_sqlAliases"), PH_NOISY|PH_READONLY);
		phalcon_read_property(&tmp_sql_aliases_models, getThis(), SL("_sqlAliasesModels"), PH_NOISY|PH_READONLY);
		phalcon_read_property(&tmp_sql_models_aliases, getThis(), SL("_sqlModelsAliases"), PH_NOISY|PH_READONLY);
		phalcon_read_property(&tmp_sql_aliases_models_instances, getThis(), SL("_sqlAliasesModelsInstances"), PH_NOISY|PH_READONLY);

		phalcon_update_property_array_merge(getThis(), SL("_models"), &models);
		phalcon_update_property_array_merge(getThis(), SL("_modelsInstances"), &models_instances);
		phalcon_update_property_array_merge(getThis(), SL("_sqlAliases"), &sql_aliases);
		phalcon_update_property_array_merge(getThis(), SL("_sqlAliasesModels"), &sql_aliases_models);
		phalcon_update_property_array_merge(getThis(), SL("_sqlModelsAliases"), &sql_models_aliases);
		phalcon_update_property_array_merge(getThis(), SL("_sqlAliasesModelsInstances"), &sql_aliases_models_instances);
	}


	/**
	 * Processing joins
	 */
	if (phalcon_array_isset_fetch_str(&joins, &select, SL("joins"), PH_READONLY)) {
		if (phalcon_fast_count_ev(&joins)) {
			PHALCON_MM_CALL_METHOD(&sql_joins, getThis(), "_getjoins", &select);
		} else {
			array_init(&sql_joins);
		}
	} else {
		array_init(&sql_joins);
	}
	PHALCON_MM_ADD_ENTRY(&sql_joins);

	/**
	 * Processing selected columns
	 */
	if (!phalcon_array_isset_long(&columns, 0)) {
		array_init_size(&select_columns, 1);
		PHALCON_MM_ADD_ENTRY(&select_columns);
		phalcon_array_append(&select_columns, &columns, PH_COPY);
	} else {
		ZVAL_COPY_VALUE(&select_columns, &columns);
	}

	/**
	 * Resolve selected columns
	 */
	ZVAL_LONG(&position, 0);

	array_init(&sql_column_aliases);
	PHALCON_MM_ADD_ENTRY(&sql_column_aliases);
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(select_columns), column) {
		zval sql_column_group = {}, *sql_column;

		PHALCON_MM_CALL_METHOD(&sql_column_group, getThis(), "_getselectcolumn", column);
		PHALCON_MM_ADD_ENTRY(&sql_column_group);
		if (likely(Z_TYPE(sql_column_group) != IS_ARRAY)) {
			continue;
		}

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(sql_column_group), sql_column) {
			zval alias = {}, type = {};
			/**
			 * If 'alias' is set, the user had defined a alias for the column
			 */
			if (phalcon_array_isset_fetch_str(&alias, column, SL("alias"), PH_READONLY)) {
				/**
				 * The best alias is the one provided by the user
				 */
				phalcon_array_update_string(sql_column, IS(balias), &alias, PH_COPY);
				phalcon_array_update_string(sql_column, IS(sqlAlias), &alias, PH_COPY);
				phalcon_array_update(&sql_columns, &alias, sql_column, PH_COPY);
				phalcon_array_update_zval_bool(&sql_column_aliases, &alias, 1, PH_COPY);
			} else {
				/**
				 * 'balias' is the best alias selected for the column
				 */
				if (phalcon_array_isset_fetch_str(&alias, sql_column, SL("balias"), PH_READONLY)) {
					phalcon_array_update(&sql_columns, &alias, sql_column, PH_COPY);
				} else {
					phalcon_array_fetch_string(&type, sql_column, IS(type), PH_NOISY|PH_READONLY);
					if (PHALCON_IS_STRING(&type, "scalar")) {
						PHALCON_CONCAT_SV(&alias, "_", &position);
						phalcon_array_update(&sql_columns, &alias, sql_column, PH_COPY);
						zval_ptr_dtor(&alias);
					} else {
						phalcon_array_append(&sql_columns, sql_column, PH_COPY);
					}
				}
			}

			phalcon_increment(&position);
		} ZEND_HASH_FOREACH_END();
	} ZEND_HASH_FOREACH_END();

	phalcon_update_property(getThis(), SL("_sqlColumnAliases"), &sql_column_aliases);

	/**
	 * sql_select is the final prepared SELECT
	 */
	array_init_size(&sql_select, 10);
	PHALCON_MM_ADD_ENTRY(&sql_select);
	if (phalcon_array_isset_fetch_str(&distinct, &select, SL("distinct"), PH_READONLY)) {
		phalcon_array_update_str(&sql_select, SL("distinct"), &distinct, PH_COPY);
	}

	phalcon_array_update_string(&sql_select, IS(models), &sql_models, PH_COPY);
	phalcon_array_update_string(&sql_select, IS(tables), &sql_tables, PH_COPY);
	phalcon_array_update_string(&sql_select, IS(columns), &sql_columns, PH_COPY);

	if (phalcon_fast_count_ev(&sql_joins)) {
		phalcon_array_update_string(&sql_select, IS(joins), &sql_joins, PH_COPY);
	}

	/**
	 * Process WHERE clause if any
	 */
	if (phalcon_array_isset_fetch_str(&where, &ast, SL("where"), PH_READONLY)) {
		PHALCON_MM_CALL_METHOD(&where_expr, getThis(), "_getexpression", &where);
		phalcon_array_update_string(&sql_select, IS(where), &where_expr, 0);
	}

	/**
	 * Process GROUP BY clause if any
	 */
	if (phalcon_array_isset_fetch_str(&group_by, &ast, SL("groupBy"), PH_READONLY)) {
		PHALCON_MM_CALL_METHOD(&sql_group, getThis(), "_getgroupclause", &group_by);
		phalcon_array_update_string(&sql_select, IS(group), &sql_group, 0);
	}

	/**
	 * Process HAVING clause if any
	 */
	if (phalcon_array_isset_fetch_str(&having, &ast, SL("having"), PH_READONLY)) {
		PHALCON_MM_CALL_METHOD(&having_expr, getThis(), "_getexpression", &having);
		phalcon_array_update_string(&sql_select, IS(having), &having_expr, 0);
	}

	/**
	 * Process ORDER BY clause if any
	 */
	if (phalcon_array_isset_fetch_str(&order, &ast, SL("orderBy"), PH_READONLY)) {
		PHALCON_MM_CALL_METHOD(&sql_order, getThis(), "_getorderclause", &order);
		phalcon_array_update_string(&sql_select, IS(order), &sql_order, 0);
	}

	/**
	 * Process LIMIT clause if any
	 */
	if (phalcon_array_isset_fetch_str(&limit, &ast, SL("limit"), PH_READONLY)) {
		PHALCON_MM_CALL_METHOD(&sql_limit, getThis(), "_getlimitclause", &limit);
		phalcon_array_update_string(&sql_select, IS(limit), &sql_limit, 0);
	}

	/**
	 * Process FOR UPDATE clause if any
	 */
	if (phalcon_array_isset_fetch_str(&forupdate, &ast, SL("forupdate"), PH_READONLY)) {
		phalcon_array_update_string(&sql_select, IS(forupdate), &forupdate, PH_COPY);
	}

	if (zend_is_true(merge)) {
		phalcon_update_property(getThis(), SL("_models"), &tmp_models);
		phalcon_update_property(getThis(), SL("_modelsInstances"), &tmp_models_instances);
		phalcon_update_property(getThis(), SL("_sqlAliases"), &tmp_sql_aliases);
		phalcon_update_property(getThis(), SL("_sqlAliasesModels"), &tmp_sql_aliases_models);
		phalcon_update_property(getThis(), SL("_sqlModelsAliases"), &tmp_sql_models_aliases);
		phalcon_update_property(getThis(), SL("_sqlAliasesModelsInstances"), &tmp_sql_aliases_models_instances);
	}

	if (_ast) {
		phalcon_update_property(getThis(), SL("_ast"), &old_ast);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterPrepareSelect");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	RETURN_MM_CTOR(&sql_select);
}

/**
 * Analyzes an INSERT intermediate code and produces an array to be executed later
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareInsert){

	zval event_name = {}, ast = {}, qualified_name = {}, values = {}, model_name = {}, manager = {}, model = {}, source = {}, schema = {}, table = {}, sql_aliases = {};
	zval expr_rows = {}, *row, sql_insert = {}, sql_fields = {}, fields = {}, *field, exception_message = {};

	PHALCON_MM_INIT();

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforePrepareInsert");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	phalcon_read_property(&ast, getThis(), SL("_ast"), PH_NOISY|PH_READONLY);

	if (!phalcon_array_isset_fetch_str(&qualified_name, &ast, SL("qualifiedName"), PH_READONLY)) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted INSERT AST: Missing 'qualifiedName'");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&values, &ast, SL("values"), PH_READONLY)) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted INSERT AST: Missing 'values'");
		return;
	}

	/**
	 * Check if the related model exists
	 */
	if (!phalcon_array_isset_fetch_str(&model_name, &qualified_name, SL("name"), PH_READONLY)) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted INSERT AST");
		return;
	}

	PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
	PHALCON_MM_ADD_ENTRY(&manager);

	PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
	PHALCON_MM_ADD_ENTRY(&model);
	PHALCON_MM_CALL_METHOD(&source, &model, "getsource", getThis());
	PHALCON_MM_ADD_ENTRY(&source);
	PHALCON_MM_CALL_METHOD(&schema, &model, "getschema", getThis());
	PHALCON_MM_ADD_ENTRY(&schema);

	if (zend_is_true(&schema)) {
		array_init_size(&table, 2);
		phalcon_array_append(&table, &source, PH_COPY);
		phalcon_array_append(&table, &schema, PH_COPY);
	} else {
		ZVAL_DUP(&table, &source);
	}
	PHALCON_MM_ADD_ENTRY(&table);

	array_init(&sql_aliases);
	PHALCON_MM_ADD_ENTRY(&sql_aliases);
	array_init(&expr_rows);
	PHALCON_MM_ADD_ENTRY(&expr_rows);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(values), row) {
		zval *expr_value, expr_values = {};

		array_init(&expr_values);
		PHALCON_MM_ADD_ENTRY(&expr_values);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(row), expr_value) {
			zval expr_insert = {};
			/**
			 * Resolve every expression in the 'values' clause
			 */
			PHALCON_MM_CALL_METHOD(&expr_insert, getThis(), "_getexpression", expr_value);

			phalcon_array_append(&expr_values, &expr_insert, 0);
		} ZEND_HASH_FOREACH_END();

		phalcon_array_append(&expr_rows, &expr_values, PH_COPY);

	} ZEND_HASH_FOREACH_END();

	array_init(&sql_insert);
	phalcon_array_update_string(&sql_insert, IS(model), &model_name, PH_COPY);
	phalcon_array_update_string(&sql_insert, IS(table), &table, PH_COPY);
	PHALCON_MM_ADD_ENTRY(&sql_insert);

	if (phalcon_array_isset_fetch_str(&fields, &ast, SL("fields"), PH_READONLY)) {
		array_init(&sql_fields);
		PHALCON_MM_ADD_ENTRY(&sql_fields);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(fields), field) {
			zval name = {}, attribute = {}, phql = {};
			phalcon_array_fetch_string(&name, field, IS(name), PH_NOISY|PH_READONLY);

			/**
			 * Check that inserted fields are part of the model
			 */
			PHALCON_MM_CALL_METHOD(&attribute, &model, "getrealattribute", &name);
			PHALCON_MM_ADD_ENTRY(&attribute);
			if (!zend_is_true(&attribute)) {
				phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);

				PHALCON_CONCAT_SVSVS(&exception_message, "The model '", &model_name, "' doesn't have the attribute '", &name, "'");
				PHALCON_SCONCAT_SV(&exception_message, ", when preparing: ", &phql);
				PHALCON_MM_ADD_ENTRY(&exception_message);
				PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
				return;
			}

			/**
			 * Add the file to the insert list
			 */
			phalcon_array_append(&sql_fields, &attribute, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_array_update_string(&sql_insert, IS(fields), &sql_fields, PH_COPY);
	} else {
		PHALCON_MM_CALL_METHOD(&sql_fields, &model, "getattributes");

		phalcon_array_update_string(&sql_insert, IS(fields), &sql_fields, 0);
	}
	phalcon_array_update_string(&sql_insert, IS(values), &expr_rows, PH_COPY);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterPrepareInsert");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	RETURN_MM_CTOR(&sql_insert);
}

/**
 * Analyzes an UPDATE intermediate code and produces an array to be executed later
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareUpdate){

	zval event_name = {}, ast = {}, update = {}, tables = {}, values = {}, models = {}, models_instances = {}, sql_tables = {}, sql_models = {}, sql_aliases = {}, sql_aliases_models_instances = {};
	zval update_tables = {}, manager = {}, *table, sql_fields = {}, sql_values = {}, update_values = {}, *update_value;
	zval where = {}, where_expr = {}, limit = {}, sql_limit = {};

	ZVAL_STRING(&event_name, "query:beforePrepareUpdate");
	PHALCON_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	zval_ptr_dtor(&event_name);

	phalcon_read_property(&ast, getThis(), SL("_ast"), PH_NOISY|PH_READONLY);
	if (!phalcon_array_isset_fetch_str(&update, &ast, SL("update"), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted UPDATE AST");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&tables, &update, SL("tables"), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted UPDATE AST");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&values, &update, SL("values"), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted UPDATE AST");
		return;
	}

	/**
	 * We use these arrays to store info related to models, alias and its sources. With
	 * them we can rename columns later
	 */
	array_init(&models);
	array_init(&models_instances);
	array_init(&sql_tables);
	array_init(&sql_models);
	array_init(&sql_aliases);
	array_init(&sql_aliases_models_instances);

	if (!phalcon_array_isset_long(&tables, 0)) {
		array_init_size(&update_tables, 1);
		phalcon_array_append(&update_tables, &tables, PH_COPY);
	} else {
		ZVAL_COPY(&update_tables, &tables);
	}

	PHALCON_CALL_METHOD(&manager, getThis(), "getmodelsmanager");

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(update_tables), table) {
		zval qualified_name = {}, model_name = {}, ns_alias = {}, real_namespace = {}, real_model_name = {}, model = {}, source = {}, schema = {}, complete_source = {}, alias = {};

		phalcon_array_fetch_str(&qualified_name, table, SL("qualifiedName"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_string(&model_name, &qualified_name, IS(name), PH_NOISY|PH_READONLY);

		/**
		 * Check if the table have a namespace alias
		 */
		if (phalcon_array_isset_fetch_str(&ns_alias, &qualified_name, SL("ns-alias"), PH_READONLY)) {
			/**
			 * Get the real namespace alias
			 */
			PHALCON_CALL_METHOD(&real_namespace, &manager, "getnamespacealias", &ns_alias);

			/**
			 * Create the real namespaced name
			 */
			PHALCON_CONCAT_VSV(&real_model_name, &real_namespace, "\\", &model_name);
			zval_ptr_dtor(&real_namespace);
		} else {
			ZVAL_COPY(&real_model_name, &model_name);
		}

		/**
		 * Load a model instance from the models manager
		 */
		PHALCON_CALL_METHOD(&model, &manager, "load", &real_model_name);
		zval_ptr_dtor(&real_model_name);
		PHALCON_CALL_METHOD(&source, &model, "getsource", getThis());
		PHALCON_CALL_METHOD(&schema, &model, "getschema", getThis());

		/**
		 * Create a full source representation including schema
		 */
		array_init(&complete_source);
		phalcon_array_append(&complete_source, &source, PH_COPY);
		if (zend_is_true(&schema)) {
			phalcon_array_append(&complete_source, &schema, PH_COPY);
		} else {
			add_next_index_null(&complete_source);
		}

		/**
		 * Check if the table is aliased
		 */
		if (phalcon_array_isset_fetch_str(&alias, table, SL("alias"), PH_READONLY)) {
			phalcon_array_append(&complete_source, &alias, PH_COPY);

			phalcon_array_append(&sql_tables, &complete_source, PH_COPY);
			phalcon_array_update(&sql_aliases, &alias, &alias, PH_COPY);
			phalcon_array_update(&sql_aliases_models_instances, &alias, &model, PH_COPY);
			phalcon_array_update(&models, &alias, &model_name, PH_COPY);
		} else {
			phalcon_array_append(&sql_tables, &complete_source, PH_COPY);
			phalcon_array_update(&sql_aliases, &model_name, &source, PH_COPY);
			phalcon_array_update(&sql_aliases_models_instances, &model_name, &model, PH_COPY);
			phalcon_array_update(&models, &model_name, &source, PH_COPY);
		}
		zval_ptr_dtor(&complete_source);
		zval_ptr_dtor(&schema);
		zval_ptr_dtor(&source);

		phalcon_array_append(&sql_models, &model_name, PH_COPY);
		phalcon_array_update(&models_instances, &model_name, &model, PH_COPY);
		zval_ptr_dtor(&model);
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(&manager);
	zval_ptr_dtor(&update_tables);

	/**
	 * Update the models/alias/sources in the object
	 */
	phalcon_update_property(getThis(), SL("_currentModelsInstances"), &models_instances);
	phalcon_update_property(getThis(), SL("_models"), &models);
	phalcon_update_property(getThis(), SL("_modelsInstances"), &models_instances);
	phalcon_update_property(getThis(), SL("_sqlAliases"), &sql_aliases);
	phalcon_update_property(getThis(), SL("_sqlAliasesModelsInstances"), &sql_aliases_models_instances);

	zval_ptr_dtor(&models);
	zval_ptr_dtor(&models_instances);
	zval_ptr_dtor(&sql_aliases);
	zval_ptr_dtor(&sql_aliases_models_instances);


	array_init(&sql_fields);
	array_init(&sql_values);

	if (!phalcon_array_isset_long(&values, 0)) {
		array_init_size(&update_values, 1);
		phalcon_array_append(&update_values, &values, PH_COPY);
	} else {
		ZVAL_COPY(&update_values, &values);
	}

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(update_values), update_value) {
		zval column = {}, sql_column = {}, expr_column = {}, expr_value = {}, type = {}, value = {};

		phalcon_array_fetch_str(&column, update_value, SL("column"), PH_NOISY|PH_READONLY);

		PHALCON_CALL_METHOD(&sql_column, getThis(), "_getexpression", &column);
		phalcon_array_append(&sql_fields, &sql_column, 0);

		phalcon_array_fetch_str(&expr_column, update_value, SL("expr"), PH_NOISY|PH_READONLY);
		PHALCON_CALL_METHOD(&expr_value, getThis(), "_getexpression", &expr_column);

		phalcon_array_fetch_string(&type, &expr_column, IS(type), PH_NOISY|PH_READONLY);

		array_init_size(&value, 2);
		phalcon_array_update_string(&value, IS(type), &type, PH_COPY);
		phalcon_array_update_str(&value, SL("value"), &expr_value, 0);
		phalcon_array_append(&sql_values, &value, 0);
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(&update_values);

	array_init(return_value);
	phalcon_array_update_string(return_value, IS(tables), &sql_tables, 0);
	phalcon_array_update_string(return_value, IS(models), &sql_models, 0);
	phalcon_array_update_string(return_value, IS(fields), &sql_fields, 0);
	phalcon_array_update_string(return_value, IS(values), &sql_values, 0);

	if (phalcon_array_isset_fetch_str(&where, &ast, SL("where"), PH_READONLY)) {
		PHALCON_CALL_METHOD(&where_expr, getThis(), "_getexpression", &where);
		phalcon_array_update_string(return_value, IS(where), &where_expr, 0);
	}

	if (phalcon_array_isset_fetch_str(&limit, &ast, SL("limit"), PH_READONLY)) {
		PHALCON_CALL_METHOD(&sql_limit, getThis(), "_getlimitclause", &limit);
		phalcon_array_update_string(return_value, IS(limit), &sql_limit, 0);
	}

	ZVAL_STRING(&event_name, "query:afterPrepareUpdate");
	PHALCON_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	zval_ptr_dtor(&event_name);
}

/**
 * Analyzes a DELETE intermediate code and produces an array to be executed later
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _prepareDelete){

	zval event_name = {}, ast = {}, delete_ast = {}, tables = {}, models = {}, models_instances = {}, sql_tables = {}, sql_models = {}, sql_aliases = {}, sql_aliases_models_instances = {};
	zval delete_tables = {}, manager = {}, *table, where = {}, where_expr = {}, limit = {}, sql_limit = {};

	ZVAL_STRING(&event_name, "query:beforePrepareDelete");
	PHALCON_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	zval_ptr_dtor(&event_name);

	phalcon_read_property(&ast, getThis(), SL("_ast"), PH_NOISY|PH_READONLY);
	if (!phalcon_array_isset_fetch_str(&delete_ast, &ast, SL("delete"), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted DELETE AST");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&tables, &delete_ast, SL("tables"), PH_READONLY)) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted DELETE AST");
		return;
	}

	/**
	 * We use these arrays to store info related to models, alias and its sources. With
	 * them we can rename columns later
	 */
	array_init(&models);
	array_init(&models_instances);
	array_init(&sql_tables);
	array_init(&sql_models);
	array_init(&sql_aliases);
	array_init(&sql_aliases_models_instances);

	if (!phalcon_array_isset_long(&tables, 0)) {
		array_init_size(&delete_tables, 1);
		phalcon_array_append(&delete_tables, &tables, PH_COPY);
	} else {
		ZVAL_COPY(&delete_tables, &tables);
	}

	PHALCON_CALL_SELF(&manager, "getmodelsmanager");

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(delete_tables), table) {
		zval qualified_name = {}, model_name = {}, ns_alias = {}, real_namespace = {}, real_model_name = {}, model = {}, source = {}, schema = {}, complete_source = {}, alias = {};

		phalcon_array_fetch_str(&qualified_name, table, SL("qualifiedName"), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_string(&model_name, &qualified_name, IS(name), PH_NOISY|PH_READONLY);

		/**
		 * Check if the table have a namespace alias
		 */
		if (phalcon_array_isset_str(&qualified_name, SL("ns-alias"))) {
			phalcon_array_fetch_str(&ns_alias, &qualified_name, SL("ns-alias"), PH_NOISY|PH_READONLY);

			/**
			 * Get the real namespace alias
			 */
			PHALCON_CALL_METHOD(&real_namespace, &manager, "getnamespacealias", &ns_alias);

			/**
			 * Create the real namespaced name
			 */
			PHALCON_CONCAT_VSV(&real_model_name, &real_namespace, "\\", &model_name);
			zval_ptr_dtor(&real_namespace);
		} else {
			ZVAL_COPY(&real_model_name, &model_name);
		}

		/**
		 * Load a model instance from the models manager
		 */
		PHALCON_CALL_METHOD(&model, &manager, "load", &real_model_name);
		zval_ptr_dtor(&real_model_name);
		PHALCON_CALL_METHOD(&source, &model, "getsource", getThis());
		PHALCON_CALL_METHOD(&schema, &model, "getschema", getThis());

		array_init(&complete_source);
		phalcon_array_append(&complete_source, &source, 0);
		if (zend_is_true(&schema)) {
			phalcon_array_append(&complete_source, &schema, 0);
		} else {
			add_next_index_null(&complete_source);
		}

		if (phalcon_array_isset_fetch_str(&alias, table, SL("alias"), PH_READONLY)) {
			phalcon_array_append(&complete_source, &alias, PH_COPY);
			phalcon_array_append(&sql_tables, &complete_source, 0);
			phalcon_array_update(&sql_aliases, &alias, &alias, PH_COPY);
			phalcon_array_update(&sql_aliases_models_instances, &alias, &model, PH_COPY);
			phalcon_array_update(&models, &alias, &model_name, PH_COPY);
		} else {
			phalcon_array_append(&sql_tables, &complete_source, 0);
			phalcon_array_update(&sql_aliases, &model_name, &source, PH_COPY);
			phalcon_array_update(&sql_aliases_models_instances, &model_name, &model, PH_COPY);
			phalcon_array_update(&models, &model_name, &source, PH_COPY);
		}

		phalcon_array_append(&sql_models, &model_name, PH_COPY);
		phalcon_array_update(&models_instances, &model_name, &model, PH_COPY);
		zval_ptr_dtor(&model);
	} ZEND_HASH_FOREACH_END();
	zval_ptr_dtor(&delete_tables);
	zval_ptr_dtor(&manager);

	/**
	 * Update the models/alias/sources in the object
	 */
	phalcon_update_property(getThis(), SL("_currentModelsInstances"), &models_instances);
	phalcon_update_property(getThis(), SL("_models"), &models);
	phalcon_update_property(getThis(), SL("_modelsInstances"), &models_instances);
	phalcon_update_property(getThis(), SL("_sqlAliases"), &sql_aliases);
	phalcon_update_property(getThis(), SL("_sqlAliasesModelsInstances"), &sql_aliases_models_instances);
	zval_ptr_dtor(&models);
	zval_ptr_dtor(&models_instances);
	zval_ptr_dtor(&sql_aliases);
	zval_ptr_dtor(&sql_aliases_models_instances);

	array_init_size(return_value, 4);
	phalcon_array_update_string(return_value, IS(tables), &sql_tables, 0);
	phalcon_array_update_string(return_value, IS(models), &sql_models, 0);

	if (phalcon_array_isset_fetch_str(&where, &ast, SL("where"), PH_READONLY)) {
		PHALCON_CALL_METHOD(&where_expr, getThis(), "_getexpression", &where);
		phalcon_array_update_string(return_value, IS(where), &where_expr, 0);
	}

	if (phalcon_array_isset_fetch_str(&limit, &ast, SL("limit"), PH_READONLY)) {
		PHALCON_CALL_METHOD(&sql_limit, getThis(), "_getlimitclause", &limit);
		phalcon_array_update_string(return_value, IS(limit), &sql_limit, 0);
	}

	ZVAL_STRING(&event_name, "query:afterPrepareDelete");
	PHALCON_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	zval_ptr_dtor(&event_name);
}

/**
 * Parses the intermediate code produced by Phalcon\Mvc\Model\Query\Lang generating another
 * intermediate representation that could be executed by Phalcon\Mvc\Model\Query
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, parse){

	zval *_phql = NULL, event_name = {}, intermediate, phql = {}, ast = {}, type = {}, ir_phql = {};
	zval exception_message = {}, debug_message = {};

	phalcon_fetch_params(1, 0, 1, &_phql);

	if (!_phql) {
		phalcon_read_property(&phql, getThis(), SL("_phql"), PH_NOISY|PH_READONLY);
	} else {
		ZVAL_COPY_VALUE(&phql, _phql);
	}

	if (unlikely(PHALCON_GLOBAL(debug).enable_debug)) {
		PHALCON_CONCAT_SV(&debug_message, "Parse PHQL: ", &phql);
		PHALCON_DEBUG_LOG(&debug_message);
		zval_ptr_dtor(&debug_message);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeParse");
	PHALCON_MM_CALL_METHOD(&intermediate, getThis(), "fireevent", &event_name, &phql);

	if (Z_TYPE(intermediate) == IS_ARRAY) {
		RETURN_MM_NCTOR(&intermediate);
	}
	zval_ptr_dtor(&intermediate);

	if (!_phql) {
		phalcon_read_property(&intermediate, getThis(), SL("_intermediate"), PH_NOISY|PH_READONLY);
		if (Z_TYPE(intermediate) == IS_ARRAY) {
			RETURN_MM_CTOR(&intermediate);
		}
	}

	/**
	 * This function parses the PHQL statement
	 */
	if (phql_parse_phql(&ast, &phql) == FAILURE) {
		RETURN_MM();
	}
	PHALCON_MM_ADD_ENTRY(&ast);

	/**
	 * A valid AST must have a type
	 */
	if (Z_TYPE(ast) != IS_ARRAY || !phalcon_array_isset_fetch_string(&type, &ast, IS(type), PH_READONLY)) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted AST");
		return;
	}

	phalcon_update_property(getThis(), SL("_ast"), &ast);
	phalcon_update_property(getThis(), SL("_type"), &type);

	switch (phalcon_get_intval(&type)) {

		case PHQL_T_SELECT:
			PHALCON_MM_CALL_METHOD(&ir_phql, getThis(), "_prepareselect");
			break;

		case PHQL_T_INSERT:
			PHALCON_MM_CALL_METHOD(&ir_phql, getThis(), "_prepareinsert");
			break;

		case PHQL_T_UPDATE:
			PHALCON_MM_CALL_METHOD(&ir_phql, getThis(), "_prepareupdate");
			break;

		case PHQL_T_DELETE:
			PHALCON_MM_CALL_METHOD(&ir_phql, getThis(), "_preparedelete");
			break;

		default:
			PHALCON_CONCAT_SVSV(&exception_message, "Unknown statement ", &type, ", when preparing: ", &phql);
			PHALCON_MM_ADD_ENTRY(&exception_message);
			PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
	}
	PHALCON_MM_ADD_ENTRY(&ir_phql);

	if (Z_TYPE(ir_phql) != IS_ARRAY) {
		PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Corrupted AST");
		return;
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterParse");
	PHALCON_MM_CALL_METHOD(return_value, getThis(), "fireevent", &event_name, &ir_phql);

	if (Z_TYPE_P(return_value) == IS_ARRAY) {
		phalcon_update_property(getThis(), SL("_intermediate"), return_value);
		RETURN_MM();
	}
	zval_ptr_dtor(return_value);
	phalcon_update_property(getThis(), SL("_intermediate"), &ir_phql);
	RETURN_MM_CTOR(&ir_phql);
}

/**
 * Sets the cache parameters of the query
 *
 * @param array $cacheOptions
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, cache){

	zval *cache_options;

	phalcon_fetch_params(0, 1, 0, &cache_options);

	phalcon_update_property(getThis(), SL("_cacheOptions"), cache_options);
	RETURN_THIS();
}

/**
 * Returns the current cache options
 *
 * @param array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getCacheOptions){


	RETURN_MEMBER(getThis(), "_cacheOptions");
}

/**
 * Returns the current cache backend instance
 *
 * @return Phalcon\Cache\BackendInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getCache){


	RETURN_MEMBER(getThis(), "_cache");
}

static inline void phalcon_query_sql_replace(zval *sql, zval *wildcard, zval *value)
{
	zval string_wildcard = {}, fixed_value = {}, sql_tmp = {};
	PHALCON_CONCAT_SVS(&string_wildcard, ":", wildcard, ",");
	if (!phalcon_memnstr(sql, &string_wildcard)) {
		zval_ptr_dtor(&string_wildcard);
		PHALCON_CONCAT_SVS(&string_wildcard, ":", wildcard, " ");
		if (!phalcon_memnstr(sql, &string_wildcard)) {
			zval_ptr_dtor(&string_wildcard);
			PHALCON_CONCAT_SVS(&string_wildcard, ":", wildcard, ")");

			if (!phalcon_memnstr(sql, &string_wildcard)) {
				zval_ptr_dtor(&string_wildcard);
				PHALCON_CONCAT_SV(&string_wildcard, ":", wildcard);
				ZVAL_COPY(&fixed_value, value);
			} else {
				PHALCON_CONCAT_VS(&fixed_value, value, ")");
			}
		} else {
			PHALCON_CONCAT_VS(&fixed_value, value, " ");
		}
	} else {
		PHALCON_CONCAT_VS(&fixed_value, value, ",");
	}
	phalcon_fast_str_replace(&sql_tmp, &string_wildcard, &fixed_value, sql);
	zval_ptr_dtor(&string_wildcard);
	zval_ptr_dtor(&fixed_value);
	ZVAL_COPY_VALUE(sql, &sql_tmp);
}

/**
 * Ignore last insert id
 *
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, ignoreLastInsertId){

	phalcon_update_property_bool(getThis(), SL("_ignoreLastInsertId"), 1);
}

/**
 * Executes the SELECT intermediate representation producing a Phalcon\Mvc\Model\Resultset
 *
 * @param array $intermediate
 * @param array $bindParams
 * @param array $bindTypes
 * @return Phalcon\Mvc\Model\ResultsetInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeSelect){

	zval event_name = {}, intermediate = {}, bind_params = {}, bind_types = {}, manager = {}, models = {}, number_models = {}, models_instances = {};
	zval model_name = {}, model = {}, instance = {}, connection = {}, *model_name2, columns = {}, *column, select_columns = {};
	zval simple_column_map = {}, dialect = {}, sql_select = {}, processed = {}, *value = NULL, processed_types = {}, tmp = {};
	zval result = {}, dependency_injector = {}, cache = {};
	zval service_name = {}, has = {}, service_params = {};
	zend_string *str_key;
	ulong idx;
	int have_scalars = 0, have_objects = 0, is_complex = 0, is_simple_std = 0;
	size_t number_objects = 0;

	PHALCON_MM_INIT();
	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeExecuteSelect");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	/**
	 * Models instances is an array if the SELECT was prepared with parse
	 */
	phalcon_read_property(&models_instances, getThis(), SL("_modelsInstances"), PH_READONLY);
	if (Z_TYPE(models_instances) != IS_ARRAY) {
		array_init(&models_instances);
		PHALCON_MM_ADD_ENTRY(&models_instances);
	}

	PHALCON_MM_CALL_SELF(&intermediate, "getintermediate");
	PHALCON_MM_SEPARATE(&intermediate);

	PHALCON_MM_CALL_SELF(&bind_params, "getmergebindparams");
	PHALCON_MM_ADD_ENTRY(&bind_params);

	PHALCON_MM_CALL_SELF(&bind_types, "getmergebindtypes");
	PHALCON_MM_SEPARATE(&bind_types);

	PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
	PHALCON_MM_ADD_ENTRY(&manager);

	PHALCON_MM_CALL_SELF(&connection, "getreadconnection", &intermediate, &bind_params, &bind_types);
	PHALCON_MM_ADD_ENTRY(&connection);

	phalcon_array_fetch_str(&models, &intermediate, SL("models"), PH_NOISY|PH_READONLY);

	phalcon_fast_count(&number_models, &models);

	if (unlikely(Z_TYPE(models) != IS_ARRAY) || Z_LVAL(number_models) <= 0) {
		RETURN_MM_NULL();
	}

	if (PHALCON_IS_LONG(&number_models, 1)) {
		/**
		 * Load first model if is not loaded
		 */
		phalcon_array_fetch_long(&model_name, &models, 0, PH_NOISY|PH_READONLY);
		if (!phalcon_array_isset_fetch(&model, &models_instances, &model_name, PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
			phalcon_array_update(&models_instances, &model_name, &model, 0);
		}
	} else {
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(models), model_name2) {
			zval model2 = {};
			if (!phalcon_array_isset_fetch(&model2, &models_instances, model_name2, PH_READONLY)) {
				PHALCON_MM_CALL_METHOD(&model2, &manager, "load", model_name2);
				phalcon_array_update(&models_instances, model_name2, &model2, 0);
			}
			if (Z_TYPE(model) != IS_OBJECT) {
				ZVAL_COPY_VALUE(&model, &model2);
			}
		} ZEND_HASH_FOREACH_END();
	}

	phalcon_array_fetch_str(&columns, &intermediate, SL("columns"), PH_NOISY|PH_READONLY);

	/**
	 * Check if the resultset have objects and how many of them have
	 */
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL(columns), column) {
		zval column_type = {};
		phalcon_array_fetch_string(&column_type, column, IS(type), PH_NOISY|PH_READONLY);
		if (PHALCON_IS_STRING(&column_type, "scalar")) {
			if (!phalcon_array_isset_str(column, SL("balias"))) {
				is_complex = 1;
			}

			have_scalars = 1;
		} else {
			have_objects = 1;
			++number_objects;
		}
	} ZEND_HASH_FOREACH_END();

	/**
	 * Check if the resultset to return is complex or simple
	 */
	if (!is_complex) {
		if (have_objects) {
			if (have_scalars) {
				is_complex = 1;
			} else if (number_objects == 1) {
				is_simple_std = 0;
			} else {
				is_complex = 1;
			}
		} else {
			is_simple_std = 1;
		}
	}

	/**
	 * Processing selected columns
	 */
	array_init(&select_columns);
	PHALCON_MM_ADD_ENTRY(&select_columns);
	array_init(&simple_column_map);
	PHALCON_MM_ADD_ENTRY(&simple_column_map);

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(columns), idx, str_key, column) {
		zval key = {}, type = {}, sql_column = {}, model_name = {}, model2= {}, column_alias = {}, sql_alias = {};
		if (str_key) {
			ZVAL_STR(&key, str_key);
		} else {
			ZVAL_LONG(&key, idx);
		}

		phalcon_array_fetch_string(&type, column, IS(type), PH_NOISY|PH_READONLY);
		phalcon_array_fetch_str(&sql_column, column, SL("column"), PH_NOISY|PH_READONLY);

		/**
		 * Complete objects are treated in a different way
		 */
		if (PHALCON_IS_STRING(&type, "object")) {
			zval attributes = {}, *attribute;
			phalcon_array_fetch_str(&model_name, column, SL("model"), PH_NOISY|PH_READONLY);

			/**
			 * Base instance
			 */
			if (!phalcon_array_isset_fetch(&model2, &models_instances, &model_name, PH_READONLY)) {
				PHALCON_MM_CALL_METHOD(&model2, &manager, "load", &model_name);
				phalcon_array_update(&models_instances, &model_name, &model2, 0);
			}
			if (Z_TYPE(instance) != IS_OBJECT) {
				ZVAL_COPY_VALUE(&instance, &model2);
			}

			PHALCON_MM_CALL_METHOD(&attributes, &model2, "getattributes");
			PHALCON_MM_ADD_ENTRY(&attributes);
			if (is_complex) {
				zval column_map = {};
				/**
				 * If the resultset is complex we open every model into their columns
				 */
				PHALCON_MM_CALL_METHOD(&column_map, &model2, "getcolumnmap");

				/**
				 * Add every attribute in the model to the generated select
				 */
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(attributes), attribute) {
					zval hidden_alias = {}, column_alias = {};

					PHALCON_CONCAT_SVSV(&hidden_alias, "_", &sql_column, "_", attribute);

					array_init_size(&column_alias, 4);
					phalcon_array_append(&column_alias, attribute, PH_COPY);
					phalcon_array_append(&column_alias, &sql_column, PH_COPY);
					phalcon_array_append(&column_alias, &hidden_alias, 0);

					phalcon_array_append(&select_columns, &column_alias, 0);
				} ZEND_HASH_FOREACH_END();

				/**
				 * We cache required meta-data to make its future access faster
				 */
				phalcon_array_update_str_multi_2(&columns, &key, SL("instance"), &model2, PH_COPY);
				phalcon_array_update_str_multi_2(&columns, &key, SL("attributes"), &attributes, PH_COPY);
				phalcon_array_update_str_multi_2(&columns, &key, SL("columnMap"), &column_map, 0);
			} else {
				/**
				 * Query only the columns that are registered as attributes in the metaData
				 */
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL(attributes), attribute) {
					zval column_alias = {};
					array_init_size(&column_alias, 3);
					phalcon_array_append(&column_alias, attribute, PH_COPY);
					phalcon_array_append(&column_alias, &sql_column, PH_COPY);

					phalcon_array_append(&select_columns, &column_alias, 0);
				} ZEND_HASH_FOREACH_END();

			}
		} else {
			/**
			 * Create an alias if the column doesn't have one
			 */
			if (Z_TYPE(key) == IS_LONG) {
				array_init_size(&column_alias, 2);
				phalcon_array_append(&column_alias, &sql_column, PH_COPY);
				phalcon_array_append(&column_alias, &PHALCON_GLOBAL(z_null), 0);
			} else {
				array_init_size(&column_alias, 3);
				phalcon_array_append(&column_alias, &sql_column, PH_COPY);
				phalcon_array_append(&column_alias, &PHALCON_GLOBAL(z_null), 0);
				phalcon_array_append(&column_alias, &key, PH_COPY);
			}
			phalcon_array_append(&select_columns, &column_alias, 0);
		}

		/**
		 * Simulate a column map
		 */
		if (!is_complex && is_simple_std) {
			if (phalcon_array_isset_fetch_str(&sql_alias, column, SL("sqlAlias"), PH_READONLY)) {
				phalcon_array_update(&simple_column_map, &sql_alias, &key, PH_COPY);
			} else {
				phalcon_array_update(&simple_column_map, &key, &key, PH_COPY);
			}
		}
	} ZEND_HASH_FOREACH_END();

	phalcon_array_update_str(&intermediate, SL("columns"), &select_columns, PH_COPY);

	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "getindex");
	if (Z_TYPE(tmp) > IS_NULL) {
		phalcon_array_update_str(&intermediate, SL("index"), &tmp, 0);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	/**
	 * The corresponding SQL dialect generates the SQL statement based accordingly with
	 * the database system
	 */
	PHALCON_MM_CALL_METHOD(&dialect, &connection, "getdialect");
	PHALCON_MM_ADD_ENTRY(&dialect);
	PHALCON_MM_CALL_METHOD(&sql_select, &dialect, "select", &intermediate);
	PHALCON_MM_ADD_ENTRY(&sql_select);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "fireevent", &event_name, &sql_select);
	PHALCON_MM_ADD_ENTRY(&tmp);

	if (Z_TYPE(tmp) == IS_STRING) {
		ZVAL_COPY_VALUE(&sql_select, &tmp);
	}

	/**
	 * Replace the placeholders
	 */
	if (Z_TYPE(bind_params) == IS_ARRAY) {
		array_init(&processed);
		PHALCON_MM_ADD_ENTRY(&processed);
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_params), idx, str_key, value) {
			zval wildcard = {};
			if (str_key) {
				ZVAL_STR(&wildcard, str_key);
			} else {
				ZVAL_LONG(&wildcard, idx);
			}

			if (Z_TYPE_P(value) == IS_OBJECT && instanceof_function(Z_OBJCE_P(value), phalcon_db_rawvalue_ce)) {
				zval tmp_value = {};
				PHALCON_MM_CALL_METHOD(&tmp_value, value, "getvalue");
				phalcon_query_sql_replace(&sql_select, &wildcard, &tmp_value);
				zval_ptr_dtor(&tmp_value);
				PHALCON_MM_ADD_ENTRY(&sql_select);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE_P(value) == IS_ARRAY) {
				zval *v, bind_keys = {}, joined_keys = {}, hidden_param = {};
				array_init(&bind_keys);
				ZVAL_LONG(&hidden_param, 0);
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), v) {
					zval k = {}, query_key = {};
					/**
					 * Key with auto bind-params
					 */
					PHALCON_CONCAT_SVV(&k, "phi", &wildcard, &hidden_param);

					PHALCON_CONCAT_SV(&query_key, ":", &k);
					phalcon_array_append(&bind_keys, &query_key, 0);
					phalcon_array_update(&processed, &k, v, PH_COPY);
					zval_ptr_dtor(&k);
					phalcon_increment(&hidden_param);
				} ZEND_HASH_FOREACH_END();

				phalcon_fast_join_str(&joined_keys, SL(", "), &bind_keys);
				zval_ptr_dtor(&bind_keys);
				phalcon_query_sql_replace(&sql_select, &wildcard, &joined_keys);
				PHALCON_MM_ADD_ENTRY(&sql_select);
				zval_ptr_dtor(&joined_keys);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE(wildcard) == IS_LONG) {
				zval string_wildcard = {};
				PHALCON_CONCAT_SV(&string_wildcard, ":", &wildcard);
				phalcon_array_update(&processed, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed, &wildcard, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_MM_ZVAL_DUP(&processed, &bind_params);
	}

	/**
	 * Replace the bind Types
	 */
	if (Z_TYPE(bind_types) == IS_ARRAY) {
		array_init(&processed_types);
		PHALCON_MM_ADD_ENTRY(&processed_types);
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_types), idx, str_key, value) {
			zval tmp = {}, string_wildcard = {};
			if (str_key) {
				ZVAL_STR(&tmp, str_key);
			} else {
				ZVAL_LONG(&tmp, idx);
			}

			if (Z_TYPE(tmp) == IS_LONG) {
				PHALCON_CONCAT_SV(&string_wildcard, ":", &tmp);
				phalcon_array_update(&processed_types, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed_types, &tmp, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_MM_ZVAL_COPY(&processed_types, &bind_types);
	}

	/**
	 * Execute the query
	 */
	PHALCON_MM_CALL_METHOD(&result, &connection, "query", &sql_select, &processed, &processed_types);
	PHALCON_MM_ADD_ENTRY(&result);

	PHALCON_MM_CALL_METHOD(&dependency_injector, getThis(), "getdi");
	PHALCON_MM_ADD_ENTRY(&dependency_injector);

	/**
	 * Choose a resultset type
	 */
	phalcon_read_property(&cache, getThis(), SL("_cache"), PH_READONLY);
	if (!is_complex) {
		zval result_object = {};
		/**
		 * Select the base object
		 */
		if (is_simple_std) {
			/**
			 * If the result is a simple standard object use an Phalcon\Mvc\Model\Row as base
			 */
			ZVAL_STR(&service_name, IS(modelsRow));
			PHALCON_MM_CALL_METHOD(&has, &dependency_injector, "has", &service_name);
			if (zend_is_true(&has)) {
				PHALCON_MM_CALL_METHOD(&result_object, &dependency_injector, "get", &service_name);
				PHALCON_MM_ADD_ENTRY(&result_object);
				PHALCON_MM_VERIFY_CLASS_EX(&result_object, phalcon_mvc_model_row_ce, phalcon_mvc_model_query_exception_ce);
			} else {
				object_init_ex(&result_object, phalcon_mvc_model_row_ce);
				PHALCON_MM_ADD_ENTRY(&result_object);
			}
		} else {
			if (Z_TYPE(instance) == IS_OBJECT) {
				ZVAL_COPY_VALUE(&result_object, &instance);
			} else {
				ZVAL_COPY_VALUE(&result_object, &model);
			}

			PHALCON_MM_CALL_METHOD(NULL, &result_object, "reset");

			/**
			 * Get the column map
			 */
			PHALCON_MM_CALL_METHOD(&simple_column_map, &model, "getcolumnmap");
			PHALCON_MM_ADD_ENTRY(&simple_column_map);
		}

		/**
		 * Simple resultsets contains only complete objects
		 */
		ZVAL_STR(&service_name, IS(modelsResultsetSimple));

		PHALCON_MM_CALL_METHOD(&has, &dependency_injector, "has", &service_name);
		if (zend_is_true(&has)) {
			array_init(&service_params);
			PHALCON_MM_ADD_ENTRY(&service_params);
			phalcon_array_append(&service_params, &simple_column_map, PH_COPY);
			phalcon_array_append(&service_params, &result_object, PH_COPY);
			phalcon_array_append(&service_params, &result, PH_COPY);
			phalcon_array_append(&service_params, &cache, PH_COPY);
			phalcon_array_append(&service_params, &model, PH_COPY);

			PHALCON_MM_CALL_METHOD(return_value, &dependency_injector, "get", &service_name, &service_params);
		} else {
			object_init_ex(return_value, phalcon_mvc_model_resultset_simple_ce);
			PHALCON_MM_CALL_METHOD(NULL, return_value, "__construct", &simple_column_map, &result_object, &result, &cache, &model);
		}
	} else {
		/**
		 * Complex resultsets may contain complete objects and scalars
		 */
		ZVAL_STR(&service_name, IS(modelsResultsetComplex));

		PHALCON_MM_CALL_METHOD(&has, &dependency_injector, "has", &service_name);
		if (zend_is_true(&has)) {
			array_init(&service_params);
			PHALCON_MM_ADD_ENTRY(&service_params);
			phalcon_array_append(&service_params, &columns, PH_COPY);
			phalcon_array_append(&service_params, &result, PH_COPY);
			phalcon_array_append(&service_params, &cache, PH_COPY);
			phalcon_array_append(&service_params, &model, PH_COPY);

			PHALCON_MM_CALL_METHOD(return_value, &dependency_injector, "get", &service_name, &service_params);
		} else {
			object_init_ex(return_value, phalcon_mvc_model_resultset_complex_ce);
			PHALCON_MM_CALL_METHOD(NULL, return_value, "__construct", &columns, &result, &cache, &model);
		}
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterExecuteSelect");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	RETURN_MM();
}

/**
 * Executes the INSERT intermediate representation producing a Phalcon\Mvc\Model\Query\Status
 *
 * @param array $intermediate
 * @param array $bindParams
 * @param array $bindTypes
 * @return Phalcon\Mvc\Model\Query\StatusInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeInsert){

	zval event_name = {}, intermediate = {}, bind_params = {}, bind_types = {}, model_name = {}, connection = {}, models_instances = {};
	zval model = {}, dialect = {}, sql_insert = {}, processed = {}, processed_types = {}, *value = NULL, tmp = {};
	zval ignore_last_lnsertid = {}, success = {}, identity_field = {}, support_sequences = {}, sequence_name = {};
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_INIT();

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeExecuteInsert");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	PHALCON_MM_CALL_SELF(&intermediate, "getintermediate");
	PHALCON_MM_SEPARATE(&intermediate);
	PHALCON_MM_CALL_SELF(&bind_params, "getmergebindparams");
	PHALCON_MM_SEPARATE(&bind_params);
	PHALCON_MM_CALL_SELF(&bind_types, "getmergebindtypes");
	PHALCON_MM_SEPARATE(&bind_types);
	PHALCON_MM_CALL_SELF(&connection, "getwriteconnection", &intermediate, &bind_params, &bind_types);
	PHALCON_MM_ADD_ENTRY(&connection);

	phalcon_array_fetch_str(&model_name, &intermediate, SL("model"), PH_NOISY|PH_READONLY);

	phalcon_read_property(&models_instances, getThis(), SL("_modelsInstances"), PH_READONLY);
	if (!phalcon_array_isset_fetch(&model, &models_instances, &model_name, PH_COPY)) {
		zval manager = {};
		PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
		PHALCON_MM_ADD_ENTRY(&manager);
		PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
	}
	PHALCON_MM_ADD_ENTRY(&model);

	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "getindex");
	if (Z_TYPE(tmp) > IS_NULL) {
		phalcon_array_update_str(&intermediate, SL("index"), &tmp, PH_COPY);
		zval_ptr_dtor(&tmp);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	PHALCON_MM_CALL_METHOD(&dialect, &connection, "getdialect");
	PHALCON_MM_ADD_ENTRY(&dialect);
	PHALCON_CALL_METHOD(&sql_insert, &dialect, "insert", &intermediate);
	PHALCON_MM_ADD_ENTRY(&sql_insert);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterGenerateSQLStatement");
	PHALCON_CALL_METHOD(&tmp, getThis(), "fireevent", &event_name, &sql_insert);

	if (Z_TYPE(tmp) == IS_STRING) {
		ZVAL_COPY(&sql_insert, &tmp);
		PHALCON_MM_ADD_ENTRY(&sql_insert);
	}
	zval_ptr_dtor(&tmp);

	/**
	 * Replace the placeholders
	 */
	if (Z_TYPE(bind_params) == IS_ARRAY) {
		array_init(&processed);
		PHALCON_MM_ADD_ENTRY(&processed);

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_params), idx, str_key, value) {
			zval wildcard = {};
			if (str_key) {
				ZVAL_STR(&wildcard, str_key);
			} else {
				ZVAL_LONG(&wildcard, idx);
			}

			if (Z_TYPE_P(value) == IS_OBJECT && instanceof_function(Z_OBJCE_P(value), phalcon_db_rawvalue_ce)) {
				zval tmp_value = {};
				PHALCON_MM_CALL_METHOD(&tmp_value, value, "getvalue");
				phalcon_query_sql_replace(&sql_insert, &wildcard, &tmp_value);
				PHALCON_MM_ADD_ENTRY(&sql_insert);
				zval_ptr_dtor(&tmp_value);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE_P(value) == IS_ARRAY) {
				zval *v, bind_keys = {}, joined_keys = {}, hidden_param = {};
				array_init(&bind_keys);
				ZVAL_LONG(&hidden_param, 0);
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), v) {
					zval k = {}, query_key = {};
					/**
					 * Key with auto bind-params
					 */
					PHALCON_CONCAT_SVV(&k, "phi", &wildcard, &hidden_param);

					PHALCON_CONCAT_SV(&query_key, ":", &k);
					phalcon_array_append(&bind_keys, &query_key, 0);
					phalcon_array_update(&processed, &k, v, PH_COPY);
					zval_ptr_dtor(&k);
					phalcon_increment(&hidden_param);
				} ZEND_HASH_FOREACH_END();
				phalcon_fast_join_str(&joined_keys, SL(", "), &bind_keys);
				zval_ptr_dtor(&bind_keys);
				phalcon_query_sql_replace(&sql_insert, &wildcard, &joined_keys);
				PHALCON_MM_ADD_ENTRY(&sql_insert);
				zval_ptr_dtor(&joined_keys);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE(wildcard) == IS_LONG) {
				zval string_wildcard = {};
				PHALCON_CONCAT_SV(&string_wildcard, ":", &wildcard);
				phalcon_array_update(&processed, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed, &wildcard, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_MM_ZVAL_DUP(&processed, &bind_params);
	}

	/**
	 * Replace the bind Types
	 */
	if (Z_TYPE(bind_types) == IS_ARRAY) {
		array_init(&processed_types);
		PHALCON_MM_ADD_ENTRY(&processed_types);
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_types), idx, str_key, value) {
			zval tmp = {}, string_wildcard = {};
			if (str_key) {
				ZVAL_STR(&tmp, str_key);
			} else {
				ZVAL_LONG(&tmp, idx);
			}

			if (Z_TYPE(tmp) == IS_LONG) {
				PHALCON_CONCAT_SV(&string_wildcard, ":", &tmp);
				phalcon_array_update(&processed_types, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed_types, &tmp, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_MM_ZVAL_DUP(&processed_types, &bind_types);
	}

	/**
	 * Execute the query
	 */
	PHALCON_MM_CALL_METHOD(&success, &connection, "execute", &sql_insert, &processed, &processed_types);
	PHALCON_MM_ADD_ENTRY(&success);

	
	phalcon_read_property(&ignore_last_lnsertid, getThis(), SL("_ignoreLastInsertId"), PH_READONLY);
	PHALCON_MM_CALL_METHOD(&identity_field, &model, "getidentityfield");
	PHALCON_MM_ADD_ENTRY(&identity_field);

	if (!zend_is_true(&ignore_last_lnsertid) && zend_is_true(&success) && PHALCON_IS_NOT_EMPTY_STRING(&identity_field)) {

		/**
		 * We check if the model have sequences
		 */
		PHALCON_MM_CALL_METHOD(&support_sequences, &connection, "supportsequences");
		if (PHALCON_IS_TRUE(&support_sequences)) {
			if (phalcon_method_exists_ex(&model, SL("getsequencename")) == SUCCESS) {
				PHALCON_MM_CALL_METHOD(&sequence_name, &model, "getsequencename");
				PHALCON_MM_ADD_ENTRY(&sequence_name);
			} else {
				zval schema = {}, source = {};
				PHALCON_MM_CALL_METHOD(&schema, &model, "getschema", getThis());
				PHALCON_MM_ADD_ENTRY(&schema);
				PHALCON_MM_CALL_METHOD(&source, &model, "getsource", getThis());
				PHALCON_MM_ADD_ENTRY(&source);

				if (PHALCON_IS_EMPTY(&schema)) {
					PHALCON_CONCAT_VSVS(&sequence_name, &source, "_", &identity_field, "_seq");
				} else {
					PHALCON_CONCAT_VSVSVS(&sequence_name, &schema, ".", &source, "_", &identity_field, "_seq");
				}
				PHALCON_MM_ADD_ENTRY(&sequence_name);
			}
		} else {
			ZVAL_NULL(&sequence_name);
		}

		/**
		 * Recover the last "insert id" and assign it to the object
		 */
		PHALCON_MM_CALL_METHOD(&success, &connection, "lastinsertid", &sequence_name);
		PHALCON_MM_ADD_ENTRY(&success);
	}

	object_init_ex(return_value, phalcon_mvc_model_query_status_ce);
	PHALCON_CALL_METHOD(NULL, return_value, "__construct", &success);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterExecuteInsert");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	RETURN_MM();
}

/**
 * Executes the UPDATE intermediate representation producing a Phalcon\Mvc\Model\Query\Status
 *
 * @param array $intermediate
 * @param array $bindParams
 * @param array $bindTypes
 * @return Phalcon\Mvc\Model\Query\StatusInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeUpdate){

	zval event_name = {}, intermediate = {}, bind_params = {}, bind_types = {}, connection = {};
	zval dialect = {}, success = {}, update_sql = {}, processed = {}, processed_types = {}, *value = NULL, tmp = {};
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_INIT();
	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeExecuteUpdate");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	PHALCON_MM_CALL_METHOD(&intermediate, getThis(), "getintermediate");
	PHALCON_MM_SEPARATE(&intermediate);
	PHALCON_CALL_METHOD(&bind_params, getThis(), "getmergebindparams");
	PHALCON_MM_ADD_ENTRY(&bind_params);
	PHALCON_CALL_METHOD(&bind_types, getThis(), "getmergebindtypes");
	PHALCON_MM_SEPARATE(&bind_types);
	PHALCON_MM_CALL_METHOD(&connection, getThis(), "getwriteconnection", &intermediate, &bind_params, &bind_types);
	PHALCON_MM_ADD_ENTRY(&connection);

	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "getindex");
	if (Z_TYPE(tmp) > IS_NULL) {
		phalcon_array_update_str(&intermediate, SL("index"), &tmp, 0);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	PHALCON_MM_CALL_METHOD(&dialect, &connection, "getdialect");
	PHALCON_MM_ADD_ENTRY(&dialect);
	PHALCON_MM_CALL_METHOD(&update_sql, &dialect, "update", &intermediate);
	PHALCON_MM_ADD_ENTRY(&update_sql);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "fireevent", &event_name, &update_sql);

	if (Z_TYPE(tmp) == IS_STRING) {
		PHALCON_MM_ZVAL_COPY(&update_sql, &tmp);
	}

	if (Z_TYPE(bind_params) == IS_ARRAY) {
		array_init(&processed);
		PHALCON_MM_ADD_ENTRY(&processed);
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_params), idx, str_key, value) {
			zval wildcard = {};
			if (str_key) {
				ZVAL_STR(&wildcard, str_key);
			} else {
				ZVAL_LONG(&wildcard, idx);
			}

			if (Z_TYPE_P(value) == IS_OBJECT && instanceof_function(Z_OBJCE_P(value), phalcon_db_rawvalue_ce)) {
				zval tmp_value = {};
				PHALCON_MM_CALL_METHOD(&tmp_value, value, "getvalue");
				phalcon_query_sql_replace(&update_sql, &wildcard, &tmp_value);
				zval_ptr_dtor(&tmp_value);
				PHALCON_MM_ADD_ENTRY(&update_sql);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE_P(value) == IS_ARRAY) {
				zval *v, bind_keys = {}, joined_keys = {}, hidden_param = {};
				array_init(&bind_keys);
				ZVAL_LONG(&hidden_param, 0);
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), v) {
					zval k = {}, query_key = {};
					/**
					 * Key with auto bind-params
					 */
					PHALCON_CONCAT_SVV(&k, "phi", &wildcard, &hidden_param);
					PHALCON_CONCAT_SV(&query_key, ":", &k);
					phalcon_array_append(&bind_keys, &query_key, 0);
					phalcon_array_update(&processed, &k, v, PH_COPY);
					zval_ptr_dtor(&k);
					phalcon_increment(&hidden_param);
				} ZEND_HASH_FOREACH_END();
				phalcon_fast_join_str(&joined_keys, SL(", "), &bind_keys);
				zval_ptr_dtor(&bind_keys);
				phalcon_query_sql_replace(&update_sql, &wildcard, &joined_keys);
				zval_ptr_dtor(&joined_keys);
				PHALCON_MM_ADD_ENTRY(&update_sql);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE(wildcard) == IS_LONG) {
				zval string_wildcard = {};
				PHALCON_CONCAT_SV(&string_wildcard, ":", &wildcard);
				phalcon_array_update(&processed, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed, &wildcard, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();
	} else {
		PHALCON_MM_ZVAL_DUP(&processed, &bind_params);
	}

	/**
	 * Replace the bind Types
	 */
	if (Z_TYPE(bind_types) == IS_ARRAY) {
		array_init(&processed_types);
		PHALCON_MM_ADD_ENTRY(&processed_types);
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_types), idx, str_key, value) {
			zval wildcard = {}, string_wildcard = {};
			if (str_key) {
				ZVAL_STR(&wildcard, str_key);
			} else {
				ZVAL_LONG(&wildcard, idx);
			}

			if (Z_TYPE(wildcard) == IS_LONG) {
				PHALCON_CONCAT_SV(&string_wildcard, ":", &wildcard);
				phalcon_array_update(&processed_types, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed_types, &wildcard, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_MM_ZVAL_COPY(&processed_types, &bind_types);
	}

	PHALCON_MM_CALL_METHOD(&success, &connection, "execute", &update_sql, &processed, &processed_types);
	PHALCON_MM_ADD_ENTRY(&success);
	if (zend_is_true(&success)) {
		if (PHALCON_GLOBAL(orm).enable_strict) {
			PHALCON_CALL_METHOD(&success, &connection, "affectedrows");
			PHALCON_MM_ADD_ENTRY(&success);
		}
	}

	object_init_ex(return_value, phalcon_mvc_model_query_status_ce);
	PHALCON_CALL_METHOD(NULL, return_value, "__construct", &success);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterExecuteUpdate");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	RETURN_MM();
}

/**
 * Executes the DELETE intermediate representation producing a Phalcon\Mvc\Model\Query\Status
 *
 * @param array $intermediate
 * @param array $bindParams
 * @param array $bindTypes
 * @return Phalcon\Mvc\Model\Query\StatusInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, _executeDelete){

	zval event_name = {}, intermediate = {}, bind_params = {}, bind_types = {};
	zval connection = {}, success = {}, dialect = {}, delete_sql = {}, processed = {}, processed_types = {}, *value, tmp = {};
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_INIT();

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeExecuteDelete");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	PHALCON_MM_CALL_METHOD(&intermediate, getThis(), "getintermediate");
	PHALCON_MM_SEPARATE(&intermediate);
	PHALCON_MM_CALL_METHOD(&bind_params, getThis(), "getmergebindparams");
	PHALCON_MM_ADD_ENTRY(&bind_params);
	PHALCON_MM_CALL_METHOD(&bind_types, getThis(), "getmergebindtypes");
	PHALCON_MM_ADD_ENTRY(&bind_types);
	PHALCON_MM_CALL_METHOD(&connection, getThis(), "getwriteconnection", &intermediate, &bind_params, &bind_types);
	PHALCON_MM_ADD_ENTRY(&connection);

	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "getindex");
	if (Z_TYPE(tmp) > IS_NULL) {
		PHALCON_MM_ADD_ENTRY(&tmp);
		phalcon_array_update_str(&intermediate, SL("index"), &tmp, PH_COPY);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);

	PHALCON_MM_CALL_METHOD(&dialect, &connection, "getdialect");
	PHALCON_MM_ADD_ENTRY(&dialect);

	PHALCON_MM_CALL_METHOD(&delete_sql, &dialect, "delete", &intermediate);
	PHALCON_MM_ADD_ENTRY(&delete_sql);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterGenerateSQLStatement");
	PHALCON_MM_CALL_METHOD(&tmp, getThis(), "fireevent", &event_name, &delete_sql);
	PHALCON_MM_ADD_ENTRY(&tmp);

	if (Z_TYPE(tmp) == IS_STRING) {
		PHALCON_MM_ZVAL_COPY(&delete_sql, &tmp);
	}

	if (Z_TYPE(bind_params) == IS_ARRAY) {
		array_init(&processed);
		PHALCON_MM_ADD_ENTRY(&processed);

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_params), idx, str_key, value) {
			zval wildcard = {};
			if (str_key) {
				ZVAL_STR(&wildcard, str_key);
			} else {
				ZVAL_LONG(&wildcard, idx);
			}

			if (Z_TYPE_P(value) == IS_OBJECT && instanceof_function(Z_OBJCE_P(value), phalcon_db_rawvalue_ce)) {
				zval tmp_value = {};
				PHALCON_MM_CALL_METHOD(&tmp_value, value, "getvalue");
				PHALCON_MM_ADD_ENTRY(&tmp_value);
				phalcon_query_sql_replace(&delete_sql, &wildcard, &tmp_value);
				PHALCON_MM_ADD_ENTRY(&delete_sql);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE_P(value) == IS_ARRAY) {
				zval *v, bind_keys = {}, joined_keys = {}, hidden_param = {};
				array_init(&bind_keys);
				ZVAL_LONG(&hidden_param, 0);
				ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(value), v) {
					zval k = {}, query_key = {};
					/**
					 * Key with auto bind-params
					 */
					PHALCON_CONCAT_SVV(&k, "phi", &wildcard, &hidden_param);

					PHALCON_CONCAT_SV(&query_key, ":", &k);
					phalcon_array_append(&bind_keys, &query_key, PH_COPY);
					zval_ptr_dtor(&query_key);
					phalcon_array_update(&processed, &k, v, PH_COPY);
					zval_ptr_dtor(&k);
					phalcon_increment(&hidden_param);
				} ZEND_HASH_FOREACH_END();
				phalcon_fast_join_str(&joined_keys, SL(", "), &bind_keys);
				zval_ptr_dtor(&bind_keys);
				phalcon_query_sql_replace(&delete_sql, &wildcard, &joined_keys);
				PHALCON_MM_ADD_ENTRY(&delete_sql);
				zval_ptr_dtor(&joined_keys);
				phalcon_array_unset(&bind_types, &wildcard, 0);
			} else if (Z_TYPE(wildcard) == IS_LONG) {
				zval string_wildcard = {};
				PHALCON_CONCAT_SV(&string_wildcard, ":", &wildcard);
				phalcon_array_update(&processed, &string_wildcard, value, PH_COPY);
				zval_ptr_dtor(&string_wildcard);
			} else {
				phalcon_array_update(&processed, &wildcard, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();
	} else {
		PHALCON_MM_ZVAL_DUP(&processed, &bind_params);
	}

	/**
	 * Replace the bind Types
	 */
	if (Z_TYPE(bind_types) == IS_ARRAY) {
		array_init(&processed_types);
		PHALCON_MM_ADD_ENTRY(&processed_types);
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(bind_types), idx, str_key, value) {
			zval tmp = {}, string_wildcard = {};
			if (str_key) {
				ZVAL_STR(&tmp, str_key);
			} else {
				ZVAL_LONG(&tmp, idx);
			}

			if (Z_TYPE(tmp) == IS_LONG) {
				PHALCON_CONCAT_SV(&string_wildcard, ":", &tmp);
				phalcon_array_update(&processed_types, &string_wildcard, value, PH_COPY);
			} else {
				phalcon_array_update(&processed_types, &tmp, value, PH_COPY);
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		PHALCON_MM_ZVAL_COPY(&processed_types, &bind_types);
	}

	PHALCON_MM_CALL_METHOD(&success, &connection, "execute", &delete_sql, &processed, &processed_types);

	if (zend_is_true(&success)) {
		if (PHALCON_GLOBAL(orm).enable_strict) {
			PHALCON_MM_CALL_METHOD(&success, &connection, "affectedrows");
		}
	}

	/**
	 * Create a status to report the deletion status
	 */
	object_init_ex(return_value, phalcon_mvc_model_query_status_ce);
	PHALCON_MM_CALL_METHOD(NULL, return_value, "__construct", &success);

	PHALCON_MM_ZVAL_STRING(&event_name, "query:afterExecuteDelete");
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "fireevent", &event_name);
	RETURN_MM();
}

/**
 * Executes a parsed PHQL statement
 *
 * @param array $bindParams
 * @param array $bindTypes
 * @return mixed
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, execute){

	zval *bind_params = NULL, *bind_types = NULL, event_name = {}, unique_row = {}, type = {}, debug_message = {};
	zval cache_options = {}, cache_key = {}, lifetime = {}, cache_service = {}, cache = {}, frontend = {}, result = {}, is_fresh = {};
	zval default_bind_params = {}, merged_params = {}, default_bind_types = {}, merged_types = {}, exception_message = {}, *value;
	zend_string *str_key;
	ulong idx;
	int cache_options_is_not_null;

	phalcon_fetch_params(1, 0, 2, &bind_params, &bind_types);

	if (!bind_params) {
		bind_params = &PHALCON_GLOBAL(z_null);
	}

	if (!bind_types) {
		bind_types = &PHALCON_GLOBAL(z_null);
	}

	PHALCON_MM_ZVAL_STRING(&event_name, "query:beforeExecute");
	PHALCON_MM_CALL_METHOD(return_value, getThis(), "fireevent", &event_name);

	if (Z_TYPE_P(return_value) >= IS_ARRAY) {
		RETURN_MM();
	} else {
		zval_ptr_dtor(return_value);
	}

	phalcon_read_property(&cache_options, getThis(), SL("_cacheOptions"), PH_NOISY|PH_READONLY);
	cache_options_is_not_null = (Z_TYPE(cache_options) != IS_NULL); /* to keep scan-build happy */

	phalcon_read_property(&unique_row, getThis(), SL("_uniqueRow"), PH_NOISY|PH_READONLY);

	ZVAL_NULL(&cache);
	if (cache_options_is_not_null) {
		zval dependency_injector = {};
		if (Z_TYPE(cache_options) != IS_ARRAY) {
			PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Invalid caching options");
			return;
		}

		/**
		 * The user must set a cache key
		 */
		if (!phalcon_array_isset_fetch_str(&cache_key, &cache_options, SL("key"), PH_READONLY)) {
			PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "A cache key must be provided to identify the cached resultset in the cache backend");
			return;
		}

		/**
		 * 'modelsCache' is the default name for the models cache service
		 */
		if (!phalcon_array_isset_fetch_str(&cache_service, &cache_options, SL("service"), PH_READONLY)) {
			ZVAL_STR(&cache_service, IS(modelsCache));
		}

		PHALCON_MM_CALL_METHOD(&dependency_injector, getThis(), "getdi", &PHALCON_GLOBAL(z_true));
		PHALCON_MM_ADD_ENTRY(&dependency_injector);

		PHALCON_MM_CALL_METHOD(&cache, &dependency_injector, "getshared", &cache_service);
		PHALCON_MM_ADD_ENTRY(&cache);
		if (Z_TYPE(cache) != IS_OBJECT) {
			PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "The cache service must be an object");
			return;
		}

		PHALCON_MM_VERIFY_INTERFACE(&cache, phalcon_cache_backendinterface_ce);

		/**
		 * By defaut use use 3600 seconds (1 hour) as cache lifetime
		 */
		if (!phalcon_array_isset_fetch_str(&lifetime, &cache_options, SL("lifetime"), PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&frontend, &cache, "getfrontend");
			PHALCON_MM_ADD_ENTRY(&frontend);

			if (Z_TYPE(frontend) == IS_OBJECT) {
				PHALCON_MM_VERIFY_INTERFACE_EX(&frontend, phalcon_cache_frontendinterface_ce, phalcon_mvc_model_query_exception_ce);
				PHALCON_MM_CALL_METHOD(&lifetime, &frontend, "getlifetime");
			} else {
				ZVAL_LONG(&lifetime, 3600);
			}
		}

		PHALCON_MM_CALL_METHOD(&result, &cache, "get", &cache_key, &lifetime);
		PHALCON_MM_ADD_ENTRY(&result);
		if (unlikely(PHALCON_GLOBAL(debug).enable_debug)) {
			PHALCON_CONCAT_SV(&debug_message, "Get model query cache: ", &cache_key);
			PHALCON_DEBUG_LOG(&debug_message);
			zval_ptr_dtor(&debug_message);
		}
		if (Z_TYPE(result) != IS_NULL) {
			if (Z_TYPE(result) != IS_OBJECT) {
				PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "The cache didn't return a valid resultset");
				return;
			}

			ZVAL_BOOL(&is_fresh, 0);
			PHALCON_MM_CALL_METHOD(NULL, &result, "setisfresh", &is_fresh);

			/**
			 * Check if only the first row must be returned
			 */
			if (zend_is_true(&unique_row)) {
				PHALCON_MM_CALL_METHOD(return_value, &result, "getfirst");
			} else {
				ZVAL_COPY(return_value, &result);
			}
			RETURN_MM();
		}

		if (unlikely(PHALCON_GLOBAL(debug).enable_debug)) {
			PHALCON_CONCAT_SV(&debug_message, "Model query Cache is null: ", &cache_key);
			PHALCON_DEBUG_LOG(&debug_message);
			zval_ptr_dtor(&debug_message);
		}

		phalcon_update_property(getThis(), SL("_cache"), &cache);
	}

	/**
	 * The statement is parsed from its PHQL string or a previously processed IR
	 */
	PHALCON_MM_CALL_METHOD(NULL, getThis(), "parse");

	phalcon_read_property(&type, getThis(), SL("_type"), PH_NOISY|PH_READONLY);

	/**
	 * Check for default bind parameters and merge them with the passed ones
	 */
	phalcon_read_property(&default_bind_params, getThis(), SL("_bindParams"), PH_NOISY|PH_READONLY);
	if (Z_TYPE(default_bind_params) == IS_ARRAY) {
		if (Z_TYPE_P(bind_params) == IS_ARRAY) {
			array_init(&merged_params);
			PHALCON_MM_ADD_ENTRY(&merged_params);

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL(default_bind_params), idx, str_key, value) {
				zval tmp = {};
				if (str_key) {
					ZVAL_STR(&tmp, str_key);
				} else {
					ZVAL_LONG(&tmp, idx);
				}
				phalcon_array_update(&merged_params, &tmp, value, PH_COPY);
			} ZEND_HASH_FOREACH_END();

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(bind_params), idx, str_key, value) {
				zval tmp = {};
				if (str_key) {
					ZVAL_STR(&tmp, str_key);
				} else {
					ZVAL_LONG(&tmp, idx);
				}
				phalcon_array_update(&merged_params, &tmp, value, PH_COPY);
			} ZEND_HASH_FOREACH_END();
		} else {
			PHALCON_MM_ZVAL_DUP(&merged_params, &default_bind_params);
		}
	} else {
		PHALCON_MM_ZVAL_DUP(&merged_params, bind_params);
	}

	phalcon_update_property(getThis(), SL("_mergeBindParams"), &merged_params);

	/**
	 * Check for default bind types and merge them with the passed ones
	 */
	phalcon_read_property(&default_bind_types, getThis(), SL("_bindTypes"), PH_NOISY|PH_READONLY);
	if (Z_TYPE(default_bind_types) == IS_ARRAY) {
		if (Z_TYPE_P(bind_types) == IS_ARRAY) {
			phalcon_fast_array_merge(&merged_types, &default_bind_types, bind_types);
			PHALCON_MM_ADD_ENTRY(&merged_types);
		} else {
			PHALCON_MM_ZVAL_DUP(&merged_types, &default_bind_types);
		}
	} else {
		PHALCON_MM_ZVAL_DUP(&merged_types, bind_types);
	}

	phalcon_update_property(getThis(), SL("_mergeBindTypes"), &merged_types);

	switch (phalcon_get_intval(&type)) {

		case PHQL_T_SELECT:
			PHALCON_MM_CALL_METHOD(&result, getThis(), "_executeselect");
			break;

		case PHQL_T_INSERT:
			PHALCON_MM_CALL_METHOD(&result, getThis(), "_executeinsert");
			break;

		case PHQL_T_UPDATE:
			PHALCON_MM_CALL_METHOD(&result, getThis(), "_executeupdate");
			break;

		case PHQL_T_DELETE:
			PHALCON_MM_CALL_METHOD(&result, getThis(), "_executedelete");
			break;

		default:
			PHALCON_CONCAT_SV(&exception_message, "Unknown statement ", &type);
			PHALCON_MM_ADD_ENTRY(&exception_message);
			PHALCON_MM_THROW_EXCEPTION_ZVAL(phalcon_mvc_model_query_exception_ce, &exception_message);
			return;
	}
	PHALCON_MM_ADD_ENTRY(&result);

	/**
	 * We store the resultset in the cache if any
	 */
	if (cache_options_is_not_null) {
		if (phalcon_get_intval(&type) == PHQL_T_SELECT) {
			zval allow_empty = {};
			if (phalcon_array_isset_fetch_str(&allow_empty, &cache_options, SL("allowEmpty"), PH_READONLY)) {
				if (zend_is_true(&allow_empty)) {
					PHALCON_MM_CALL_METHOD(NULL, &cache, "save", &cache_key, &result, &lifetime);
				} else {
					if (Z_TYPE(result) == IS_OBJECT) {
						zval num = {};
						PHALCON_MM_CALL_METHOD(&num, &result, "count");
						if (PHALCON_GT_LONG(&num, 0)) {
							PHALCON_MM_CALL_METHOD(NULL, &cache, "save", &cache_key, &result, &lifetime);
						}
					}
				}
			} else {
				PHALCON_MM_CALL_METHOD(NULL, &cache, "save", &cache_key, &result, &lifetime);
			}
		}
	}

	/**
	 * Check if only the first row must be returned
	 */
	if (Z_TYPE(result) == IS_OBJECT && zend_is_true(&unique_row)) {
		zval first = {};
		PHALCON_MM_CALL_METHOD(&first, &result, "getfirst");
		PHALCON_MM_ADD_ENTRY(&first);

		PHALCON_MM_ZVAL_STRING(&event_name, "query:afterExecute");
		PHALCON_MM_CALL_METHOD(return_value, getThis(), "fireevent", &event_name, &first);

		if (Z_TYPE_P(return_value) >= IS_ARRAY) {
			RETURN_MM();
		}
		zval_ptr_dtor(return_value);
		RETURN_MM_CTOR(&first);
	} else {
		PHALCON_MM_ZVAL_STRING(&event_name, "query:afterExecute");
		PHALCON_MM_CALL_METHOD(return_value, getThis(), "fireevent", &event_name, &result);

		if (Z_TYPE_P(return_value) >= IS_ARRAY) {
			RETURN_MM();
		}
		zval_ptr_dtor(return_value);
		RETURN_MM_CTOR(&result);
	}
}

/**
 * Executes the query returning the first result
 *
 * @param array $bindParams
 * @param array $bindTypes
 * @return Phalcon\Mvc\ModelInterface
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getSingleResult){

	zval *bind_params = NULL, *bind_types = NULL, unique_row = {}, first_result = {}, result = {};

	phalcon_fetch_params(0, 0, 2, &bind_params, &bind_types);

	if (!bind_params) {
		bind_params = &PHALCON_GLOBAL(z_null);
	}

	if (!bind_types) {
		bind_types = &PHALCON_GLOBAL(z_null);
	}

	phalcon_read_property(&unique_row, getThis(), SL("_uniqueRow"), PH_NOISY|PH_READONLY);

	/**
	 * The query is already programmed to return just one row
	 */
	PHALCON_CALL_METHOD(&result, getThis(), "execute", bind_params, bind_types);

	if (zend_is_true(&unique_row)) {
		RETURN_ZVAL(&result, 0, 0);
	} else {
		PHALCON_CALL_METHOD(&first_result, &result, "getfirst");
		zval_ptr_dtor(&result);
		RETURN_ZVAL(&first_result, 0, 0);
	}
}

/**
 * Sets the type of PHQL statement to be executed
 *
 * @param int $type
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setType){

	zval *type;

	phalcon_fetch_params(0, 1, 0, &type);

	phalcon_update_property(getThis(), SL("_type"), type);
	RETURN_THIS();
}

/**
 * Gets the type of PHQL statement executed
 *
 * @return int
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getType){


	RETURN_MEMBER(getThis(), "_type");
}

/**
 * Get bind parameter
 *
 * @param string $name
 * @return mixed
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getBindParam){

	zval *name, param = {};

	phalcon_fetch_params(0, 1, 0, &name);

	phalcon_read_property_array(&param, getThis(), SL("_bindParams"), name, PH_READONLY);

	RETURN_CTOR(&param);
}

/**
 * Set default bind parameters
 *
 * @param array $bindParams
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setBindParams){

	zval *bind_params;

	phalcon_fetch_params(0, 1, 0, &bind_params);

	if (Z_TYPE_P(bind_params) != IS_ARRAY && Z_TYPE_P(bind_params) != IS_NULL) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Bind parameters must be an array");
		return;
	}
	phalcon_update_property(getThis(), SL("_bindParams"), bind_params);

	RETURN_THIS();
}

/**
 * Returns default bind params
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getBindParams){


	RETURN_MEMBER(getThis(), "_bindParams");
}

/**
 * Set merge bind parameters
 *
 * @param array $bindParams
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setMergeBindParams){

	zval *bind_params;

	phalcon_fetch_params(0, 1, 0, &bind_params);

	if (Z_TYPE_P(bind_params) != IS_ARRAY && Z_TYPE_P(bind_params) != IS_NULL) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Bind parameters must be an array");
		return;
	}
	phalcon_update_property(getThis(), SL("_mergeBindParams"), bind_params);

	RETURN_THIS();
}

/**
 * Returns merge bind params
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getMergeBindParams){


	RETURN_MEMBER(getThis(), "_mergeBindParams");
}

/**
 * Adds the index
 *
 * @param string $index
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setIndex) {

	zval *index;

	phalcon_fetch_params(0, 1, 0, &index);

	phalcon_update_property(getThis(), SL("_index"), index);
	RETURN_THIS();
}

/**
 * Gets the index
 *
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getIndex){


	RETURN_MEMBER(getThis(), "_index");
}

/**
 * Set bind type
 *
 * @param string $name
 * @param int $type
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setBindType){

	zval *name, *type;

	phalcon_fetch_params(0, 2, 0, &name, &type);

	phalcon_update_property_array(getThis(), SL("_bindTypes"), name, type);

	RETURN_THIS();
}

/**
 * Set default bind types
 *
 * @param array $bindTypes
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setBindTypes){

	zval *bind_types;

	phalcon_fetch_params(0, 1, 0, &bind_types);

	if (Z_TYPE_P(bind_types) != IS_ARRAY && Z_TYPE_P(bind_types) != IS_NULL) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Bind types must be an array");
		return;
	}
	phalcon_update_property(getThis(), SL("_bindTypes"), bind_types);

	RETURN_THIS();
}

/**
 * Returns default bind types
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getBindTypes){


	RETURN_MEMBER(getThis(), "_bindTypes");
}

/**
 * Set merge bind types
 *
 * @param array $bindTypes
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setMergeBindTypes){

	zval *bind_types;

	phalcon_fetch_params(0, 1, 0, &bind_types);

	if (Z_TYPE_P(bind_types) != IS_ARRAY && Z_TYPE_P(bind_types) != IS_NULL) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Bind types must be an array");
		return;
	}
	phalcon_update_property(getThis(), SL("_mergeBindTypes"), bind_types);

	RETURN_THIS();
}

/**
 * Returns default bind types
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getMergeBindTypes){


	RETURN_MEMBER(getThis(), "_mergeBindTypes");
}

/**
 * Allows to set the IR to be executed
 *
 * @param array $intermediate
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setIntermediate){

	zval *intermediate;

	phalcon_fetch_params(0, 1, 0, &intermediate);

	phalcon_update_property(getThis(), SL("_intermediate"), intermediate);
	RETURN_THIS();
}

/**
 * Returns the intermediate representation of the PHQL statement
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getIntermediate){


	RETURN_MEMBER(getThis(), "_intermediate");
}

/**
 * Returns the models of the PHQL statement
 *
 * @return array
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getModels){


	RETURN_MEMBER(getThis(), "_models");
}

/**
 * Sets the connection
 *
 * @param Phalcon\Db\AdapterInterface $connection
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setConnection){

	zval *connection;

	phalcon_fetch_params(0, 1, 0, &connection);
	if (Z_TYPE_P(connection) == IS_OBJECT) {
		PHALCON_VERIFY_INTERFACE(connection, phalcon_db_adapterinterface_ce);
	}
	phalcon_update_property(getThis(), SL("_connection"), connection);

	RETURN_THIS();
}

/**
 * Gets the connection
 *
 * @return mixed
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getConnection){

	zval *intermediate = NULL, *bind_params = NULL, *bind_types = NULL;

	phalcon_fetch_params(0, 0, 3, &intermediate, &bind_params, &bind_types);

	RETURN_MEMBER(getThis(), "_connection");
}

/**
 * Gets the read connection
 *
 * @param array $intermediate
 * @param array $bindParams
 * @param array $bindTypes
 * @return mixed
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getReadConnection){

	zval *intermediate = NULL, *bind_params = NULL, *bind_types = NULL, connection = {};
	zval manager = {}, models_instances = {}, models = {};
	zval number_models = {}, model_name = {}, model = {}, connections = {}, *model_key, connection_type = {}, connection_types = {};

	phalcon_fetch_params(1, 0, 3, &intermediate, &bind_params, &bind_types);

	if (!intermediate) {
		intermediate = &PHALCON_GLOBAL(z_null);
	}

	if (!bind_params) {
		bind_params = &PHALCON_GLOBAL(z_null);
	}

	if (!bind_types) {
		bind_types = &PHALCON_GLOBAL(z_null);
	}

	if (phalcon_method_exists_ex(getThis(), SL("selectreadconnection")) == SUCCESS) {
		PHALCON_MM_CALL_METHOD(&connection, getThis(), "selectreadconnection", intermediate, bind_params, bind_types);
		PHALCON_MM_ADD_ENTRY(&connection);
	}

	if (Z_TYPE(connection) == IS_OBJECT) {
		PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
		RETURN_MM_CTOR(&connection);
	}

	PHALCON_MM_CALL_SELF(&connection, "getconnection", intermediate, bind_params, bind_types);
	PHALCON_MM_ADD_ENTRY(&connection);
	if (Z_TYPE(connection) == IS_OBJECT) {
		RETURN_MM_CTOR(&connection);
	}

	PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
	PHALCON_MM_ADD_ENTRY(&manager);

	phalcon_read_property(&models_instances, getThis(), SL("_modelsInstances"), PH_READONLY);
	if (Z_TYPE(models_instances) != IS_ARRAY) {
		array_init(&models_instances);
		PHALCON_MM_ADD_ENTRY(&models_instances);
	}

	if (!phalcon_array_isset_fetch_str(&models, intermediate, SL("models"), PH_READONLY) 
		&& phalcon_array_isset_fetch_str(&model_name, intermediate, SL("model"), PH_READONLY)) {
		if (!phalcon_array_isset_fetch(&model, &models_instances, &model_name, PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
			phalcon_array_update(&models_instances, &model_name, &model, 0);
		}

		PHALCON_MM_CALL_METHOD(&connection, &model, "getreadconnection", getThis(), intermediate, bind_params, bind_types);
		PHALCON_MM_ADD_ENTRY(&connection);

		PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
		RETURN_MM_CTOR(&connection);
	}

	phalcon_fast_count(&number_models, &models);
	if (PHALCON_IS_LONG(&number_models, 1)) {
		phalcon_array_fetch_long(&model_name, &models, 0, PH_NOISY|PH_READONLY);
		if (!phalcon_array_isset_fetch(&model, &models_instances, &model_name, PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
			phalcon_array_update(&models_instances, &model_name, &model, 0);
		}

		PHALCON_MM_CALL_METHOD(&connection, &model, "getreadconnection", getThis(), intermediate, bind_params, bind_types);
		PHALCON_MM_ADD_ENTRY(&connection);
		PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
		RETURN_MM_CTOR(&connection);
	} else {
		array_init(&connections);
		PHALCON_MM_ADD_ENTRY(&connections);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(models), model_key) {
			zval tmp_conn = {};
			if (!phalcon_array_isset_fetch(&model, &models_instances, model_key, PH_READONLY)) {
				PHALCON_MM_CALL_METHOD(&model, &manager, "load", model_key);
				phalcon_array_update(&models_instances, model_key, &model, 0);
			}

			PHALCON_MM_CALL_METHOD(&tmp_conn, &model, "getreadconnection", getThis(), intermediate, bind_params, bind_types);
			PHALCON_MM_ADD_ENTRY(&tmp_conn);

			if (Z_TYPE(connection) != IS_OBJECT) {
				PHALCON_MM_ZVAL_COPY(&connection, &tmp_conn);
			}

			PHALCON_MM_CALL_METHOD(&connection_type, &tmp_conn, "gettype");
			PHALCON_MM_ADD_ENTRY(&connection_type);

			phalcon_array_update_zval_bool(&connections, &connection_type, 1, 0);

			phalcon_fast_count(&connection_types, &connections);

			if (PHALCON_IS_LONG(&connection_types, 2)) {

				PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Cannot use models of different database systems in the same query");
				return;
			}
		} ZEND_HASH_FOREACH_END();
	}

	PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
	RETURN_MM_CTOR(&connection);
}

/**
 * Gets the write connection
 *
 * @param array $intermediate
 * @param array $bindParams
 * @param array $bindTypes
 * @return mixed
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, getWriteConnection){

	zval *intermediate = NULL, *bind_params = NULL, *bind_types = NULL, connection = {}, manager = {};
	zval models_instances = {}, models = {};
	zval number_models = {}, model_name = {}, model = {}, connections = {}, *model_key, connection_type = {}, connection_types = {};

	phalcon_fetch_params(1, 0, 3, &intermediate, &bind_params, &bind_types);

	if (!intermediate) {
		intermediate = &PHALCON_GLOBAL(z_null);
	}

	if (!bind_params) {
		bind_params = &PHALCON_GLOBAL(z_null);
	}

	if (!bind_types) {
		bind_types = &PHALCON_GLOBAL(z_null);
	}

	if (phalcon_method_exists_ex(getThis(), SL("selectwriteconnection")) == SUCCESS) {
		PHALCON_MM_CALL_METHOD(&connection, getThis(), "selectwriteconnection", intermediate, bind_params, bind_types);
		PHALCON_MM_ADD_ENTRY(&connection);
	}

	if (Z_TYPE(connection) == IS_OBJECT) {
		PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
		RETURN_MM_CTOR(&connection);
	}

	PHALCON_MM_CALL_SELF(&connection, "getconnection", intermediate, bind_params, bind_types);
	PHALCON_MM_ADD_ENTRY(&connection);
	if (Z_TYPE(connection) == IS_OBJECT) {
		RETURN_MM_CTOR(&connection);
	}

	PHALCON_MM_CALL_SELF(&manager, "getmodelsmanager");
	PHALCON_MM_ADD_ENTRY(&manager);

	phalcon_read_property(&models_instances, getThis(), SL("_modelsInstances"), PH_READONLY);
	if (Z_TYPE(models_instances) != IS_ARRAY) {
		array_init(&models_instances);
		PHALCON_MM_ADD_ENTRY(&models_instances);
	}

	if (!phalcon_array_isset_fetch_str(&models, intermediate, SL("models"), PH_READONLY) 
		&& phalcon_array_isset_fetch_str(&model_name, intermediate, SL("model"), PH_READONLY)) {
		if (!phalcon_array_isset_fetch(&model, &models_instances, &model_name, PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
			phalcon_array_update(&models_instances, &model_name, &model, 0);
		}

		PHALCON_MM_CALL_METHOD(&connection, &model, "getwriteconnection", getThis(), intermediate, bind_params, bind_types);
		PHALCON_MM_ADD_ENTRY(&connection);

		PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
		RETURN_MM_CTOR(&connection);
	}

	phalcon_fast_count(&number_models, &models);
	if (PHALCON_IS_LONG(&number_models, 1)) {
		phalcon_array_fetch_long(&model_name, &models, 0, PH_NOISY|PH_READONLY);
		if (!phalcon_array_isset_fetch(&model, &models_instances, &model_name, PH_READONLY)) {
			PHALCON_MM_CALL_METHOD(&model, &manager, "load", &model_name);
			phalcon_array_update(&models_instances, &model_name, &model, 0);
		}

		PHALCON_MM_CALL_METHOD(&connection, &model, "getwriteconnection", getThis(), intermediate, bind_params, bind_types);
		PHALCON_MM_ADD_ENTRY(&connection);
		PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
		RETURN_MM_CTOR(&connection);
	} else {
		array_init(&connections);
		PHALCON_MM_ADD_ENTRY(&connections);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL(models), model_key) {
			zval tmp_conn = {};
			if (!phalcon_array_isset_fetch(&model, &models_instances, model_key, PH_READONLY)) {
				PHALCON_MM_CALL_METHOD(&model, &manager, "load", model_key);
				phalcon_array_update(&models_instances, model_key, &model, 0);
			}

			PHALCON_MM_CALL_METHOD(&tmp_conn, &model, "getwriteconnection", getThis(), intermediate, bind_params, bind_types);
			PHALCON_MM_ADD_ENTRY(&tmp_conn);

			if (Z_TYPE(connection) != IS_OBJECT) {
				PHALCON_MM_ZVAL_COPY(&connection, &tmp_conn);
			}

			PHALCON_MM_CALL_METHOD(&connection_type, &tmp_conn, "gettype");
			PHALCON_MM_ADD_ENTRY(&connection_type);

			phalcon_array_update_zval_bool(&connections, &connection_type, 1, 0);

			phalcon_fast_count(&connection_types, &connections);

			if (PHALCON_IS_LONG(&connection_types, 2)) {

				PHALCON_MM_THROW_EXCEPTION_STR(phalcon_mvc_model_query_exception_ce, "Cannot use models of different database systems in the same query");
				return;
			}
		} ZEND_HASH_FOREACH_END();
	}

	PHALCON_MM_VERIFY_INTERFACE(&connection, phalcon_db_adapterinterface_ce);
	RETURN_MM_CTOR(&connection);
}

/**
 * Sets conflict
 *
 * @param array $conflict
 * @return Phalcon\Mvc\Model\Query
 */
PHP_METHOD(Phalcon_Mvc_Model_Query, setConflict){

	zval *conflict;

	phalcon_fetch_params(0, 1, 0, &conflict);

	phalcon_update_property(getThis(), SL("_conflict"), conflict);
	RETURN_THIS();
}
