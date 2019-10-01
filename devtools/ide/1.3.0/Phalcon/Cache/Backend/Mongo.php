<?php 

namespace Phalcon\Cache\Backend {

	/**
	 * Phalcon\Cache\Backend\Mongo
	 *
	 * Allows to cache output fragments, PHP data or raw data to a MongoDb backend
	 *
	 *<code>
	 *
	 * // Cache data for 2 days
	 * $frontCache = new Phalcon\Cache\Frontend\Base64(array(
	 *		"lifetime" => 172800
	 * ));
	 *
	 * //Create a MongoDB cache
	 * $cache = new Phalcon\Cache\Backend\Mongo($frontCache, array(
	 *		'uri' => "mongodb://localhost:27017",
	 *      'db' => 'caches',
	 *		'collection' => 'images'
	 * ));
	 *
	 * //Cache arbitrary data
	 * $cache->save('my-data', file_get_contents('some-image.jpg'));
	 *
	 * //Get data
	 * $data = $cache->get('my-data');
	 *
	 *</code>
	 */
	
	class Mongo extends \Phalcon\Cache\Backend implements \Phalcon\Cache\BackendInterface, \Phalcon\Di\InjectionAwareInterface, \Phalcon\Events\EventsAwareInterface {

		protected $_uri;

		protected $_db;

		protected $_collection;

		/**
		 * \Phalcon\Cache\Backend\Mongo constructor
		 *
		 * @param \Phalcon\Cache\FrontendInterface $frontend
		 * @param array $options
		 */
		public function __construct($frontend, $options=null){ }


		/**
		 * Returns a cached content
		 *
		 * @param string $keyName
		 * @return mixed
		 */
		public function get($keyName){ }


		/**
		 * Stores cached content into the Mongo backend and stops the frontend
		 *
		 * @param string $keyName
		 * @param string $content
		 * @param long $lifetime
		 * @param boolean $stopBuffer
		 */
		public function save($keyName=null, $value=null, $lifetime=null, $stopBuffer=null){ }


		/**
		 * Deletes a value from the cache by its key
		 *
		 * @param string $keyName
		 * @return boolean
		 */
		public function delete($keyName){ }


		/**
		 * Query the existing cached keys
		 *
		 * @param string $prefix
		 * @return array
		 */
		public function queryKeys($prefix=null){ }


		/**
		 * Checks if cache exists and it hasn't expired
		 *
		 * @param string $keyName
		 * @return boolean
		 */
		public function exists($keyName){ }


		public function gc(){ }


		/**
		 * Increment of a given key by $value
		 *
		 * @param int|string $keyName
		 * @param long $value
		 * @return  mixed
		 */
		public function increment($keyName, $value=null){ }


		/**
		 * Decrement of a given key by $value
		 *
		 * @param int|string $keyName
		 * @param long $value
		 * @return mixed
		 */
		public function decrement($keyName, $value=null){ }


		/**
		 * Immediately invalidates all existing items.
		 *
		 * @return bool
		 */
		public function flush(){ }

	}
}
