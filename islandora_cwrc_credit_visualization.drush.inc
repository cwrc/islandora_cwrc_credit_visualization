<?php                                                                             
 
/**
 * @file
   Custom CWRC drush scripts.
 */
 
/**
 * Implements hook_drush_command().
 */
function islandora_cwrc_credit_visualization_drush_command()
{
 
  $items = array();
 
  $items['islandora_cwrc_credit_visualization_datastore'] = array(
    'description' => 'Initialize datastore for the CWRC Credit Visualization',
    'examples' => array(
      'drush islandora_cwrc_credit_visualization_datastore {pid}'
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX
  );
 
  return $items;
}
 
 
/**
 * Credit Visualization: bootstrap or manually update datastore
 *
 */
function drush_islandora_cwrc_credit_visualization_datastore($pid)
{
  try
  {
    module_load_include('inc', 'islandora_cwrc_credit_visualization', 'includes/credit_viz_helpers');

    drush_print("results:");
    islandora_cwrc_viz_one_PID($pid);

  } catch (Exception $e) {
     // print exception
     print $e->getMessage();                                                    
  }
}


