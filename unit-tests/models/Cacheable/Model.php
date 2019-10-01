<?php

namespace Cacheable;

class Model extends \Phalcon\Mvc\Model
{

	public static function getCacheableParams($parameters)
	{

		if (!$parameters) {
			return $parameters;
		}

		if (isset($parameters['di'])) {
			unset ($parameters['di']);
		}

		$key = md5(get_called_class() . serialize($parameters));

		if (!is_array($parameters)) {
			$parameters = array($parameters);
		}

		$parameters['cache'] = array(
			'key' => $key,
			'lifetime' => 3600
		);

		return $parameters;
	}

	public static function findFirst($conditions = NULL, array $bindParams = NULL, array $options = NULL, bool $autoCreate = NULL)
	{
		$parameters = self::getCacheableParams($conditions);
		return parent::findFirst($parameters);
	}

	public static function find($conditions = NULL, array $bindParams = NULL, array $options = NULL)
	{
		$parameters = self::getCacheableParams($conditions);
		return parent::find($parameters);
	}

}