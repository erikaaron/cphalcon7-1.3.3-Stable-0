<?php

class Issue787Task extends \Phalcon\Cli\Task
{
	static $output = '';

	public function beforeExecuteRoute()
	{
		$this->dispatcher;
		$this->dispatcher;
		self::$output .= "beforeExecuteRoute" . PHP_EOL;
		return true;
	}

	public function initialize()
	{
		self::$output .= "initialize" . PHP_EOL;
	}

	public function mainAction()
	{
	}
}