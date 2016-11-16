<?php

/**
* @file 
* class to interact with the php_listeners to build the Credit Visualization 
* datastore
*/

class islandora_cwrc_credit_visualization_listener
{

  function __construct($config)
  {
      $this->config = $config;
  }

  function __destruct() {
  }
    

  /**
  * given a Fedora object, read the object workflow, extract the metadata,
  * and create a Credit Visualization datastore
  *
  * @param string $fedora_object
  *  the Fedora object as a Tuque object
  *
  **/
  function islandora_cwrc_credit_visualization_handler($fedora_object, $msgDateTime=null)
  {
    // ugly kludge - otherwise have to remove Drupal functions from the 
    // cwrc_entities include
    exec('/usr/bin/drush --root='.$this->config->drupalPath.' -u 1 '.$this->config->drushScriptName.' '.$fedora_object->id.';');
  }


}
