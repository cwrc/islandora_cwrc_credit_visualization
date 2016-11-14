<?php


/**
 * @file
 * Purpose: provide helper functions for the CWRC Credit Visualization 
 *
 */

define('ISLANDORA_CWRC_CREDIT_VIZ_WORKFLOW', 'WORKFLOW');
define('ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE', 'islandora_cwrc_credit_viz_datastore');
define('ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE', 'islandora_cwrc_credit_viz_collectionstore');

define('ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID', 'pid');
define('ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION', 'collectionId');
define('ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER', 'userId');

/**
 * prepare the JSON output for the credit visualization tool
 *
 */
function islandora_plotit_cwrc_credit_viz_api()
{
  $req = $_REQUEST;
  $ret = null;
  $queryResult = null;
  $docArray = array();

  if ( array_key_exists(ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID, $req) )
  {
    $tmp = $req[ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID];
    $pidArray = null;
    if (is_array($tmp))
    {
      $pidArray = $tmp;
    }
    else 
    {
      $pidArray = array($tmp);
    }

    $ret['query'] = array(ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID => $pidArray);

    // all workflow from the specified documents the specified collection 
    // needs sorting for version difference calculation
    $query_result = db_query("SELECT * FROM {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} WHERE object_pid IN (:pids) ORDER BY object_pid, wf_date, wf_time, workflow_id ",
      array(':pids' => $pidArray)
    );
  }
  else if ( array_key_exists(ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION, $req) )
  {
    $tmp = $req[ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION];
    $pidArray = null;
    if (is_array($tmp))
    {
      $pidArray = $tmp;
    }
    else 
    {
      $pidArray = array($tmp);
    }

    $ret['query'] = array(ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION => $pidArray);

    // all workflow from documents at are members of the specified collection 
    // needs sorting for version difference calculation
    $query_result = db_query(
      "SELECT ds.* "
      . "FROM {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} ds, {".ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE."} cs "
      . " WHERE cs.object_pid=ds.object_pid and cs.member_collection_pid IN (:pids) ORDER BY object_pid, wf_date, wf_time, workflow_id ",
      array(':pids' => $pidArray)
    );
  }
  else if ( array_key_exists(ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER, $req) )
  {
    $tmp = $req[ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER];
    $tmpArray = null;
    if (is_array($tmp))
    {
      $tmpArray = $tmp;
    }
    else 
    {
      $tmpArray = array($tmp);
    }

    $ret['query'] = array(ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER => $tmpArray);


    // all workflow from documents touched by the specified users
    // needs sorting for version difference calculation
    $query_result = db_query("SELECT * FROM {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} WHERE object_pid IN (SELECT object_pid FROM  {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} WHERE user_id IN (:user_ids) ) ORDER BY object_pid, wf_date, wf_time, workflow_id ",
      array(':user_ids' => $tmpArray)
    );

  }

  if ($query_result)
  {
    $lastPID = null;

    $detailsArray;
    $modArray;
    $cnt = 0;

    do 
    {
    
      $row = $query_result->fetchAssoc();
      
      if (!$row or ($row and $lastPID and $lastPID!=$row['object_pid']) )
      {
        $docArray[] = $detArray;
        $cnt = 0;
      }

      if ($row)
      {
        if ($lastPID==null or $lastPID!=$row['object_pid'])
        {
          $detArray = null;
          $modArray = null;
          $detArray['id'] = $row['object_pid'];
          $detArray['name'] = $row['object_label'];
          $lastPID = $row['object_pid'];
        }

        $modArray['user']['id'] = $row['user_id'];
        $modArray['user']['name'] = $row['user_name'];
        $modArray['workflow_changes']['category'] = $row['wf_category'];
        $modArray['workflow_changes']['stamp'] = $row['wf_stamp'];
        $modArray['workflow_changes']['status'] = $row['wf_status'];
        $modArray['diff_changes'][] = $row['delta_by_size'];

        $detArray['modifications'][] = $modArray;
      }

    } while ($row);
  }

  $ret['documents'] = $docArray;

  //print(json_encode($ret));
  drupal_json_output($ret);

}

/**
 * process the workflow datastream for one doc 
 */
function islandora_plotit_cwrc_viz_one_PID($pid)
{

  $fedora_object = islandora_object_load($pid);
  drush_print($fedora_object->id);

  $wfDS = $fedora_object[ISLANDORA_CWRC_CREDIT_VIZ_WORKFLOW];

  //drush_print($wfDS->content);

  if ($wfDS->content)
  {
    $wfObj = new DOMDocument();
    $wfObj->loadXML($wfDS->content);
    //print($wfObj->saveXML());
    $xpath = new DOMXPath($wfObj);
    //$xpath->registerNamespace("php", "http://php.net/xpath");
    //$xpath->registerPhpFunctions();
    $wfNodeArray = $xpath->query('/cwrc/workflow');

    $wfDocArray = array();
    $cnt = 0;
    foreach ($wfNodeArray as $wfNode)
    {

      $wfID = ($wfNode->getAttribute("workflowID"));
      if (!$wfID)
      {
        // if no workflow id
        $wfID = $pid . "_wk_" . $cnt;
      }

      $wfTmpArray['workflow_id'] = $wfID; 
      $wfTmpArray['object_pid'] = $fedora_object->id;
      $wfTmpArray['object_label'] = $fedora_object->label;
      //$wfTmpArray['collection_id'] = json_encode($fedora_object->getParents()); 
      $wfTmpArray['user_id'] = $wfNode->getAttribute("userID"); 
      //ToDo: read from Drupal
      $wfTmpArray['user_name'] = $wfNode->getAttribute("userID"); 
      $wfTmpArray['user_uri'] =  '';

      // ToDo: what timezone is used
      $wfTmpArray['wf_date'] = $wfNode->getAttribute("date"). ' ' . $wfNode->getAttribute("time"); 
      $wfTmpArray['wf_time'] = $wfNode->getAttribute("time"); 

      // should only have one activity node
      $wfActivityNode = $xpath->query('activity',$wfNode)->item(0);

      if ($wfActivityNode)
      {
        $wfTmpArray['wf_category'] = $wfActivityNode->getAttribute("category"); 
        $wfTmpArray['wf_stamp'] = $wfActivityNode->getAttribute("stamp"); 
        $wfTmpArray['wf_status'] = $wfActivityNode->getAttribute("status"); 
      }
      
      $wfTmpArray['delta_by_size'] = 0; 

      $wfDocArray[] = $wfTmpArray;
      $cnt++;

      //$tmpAttrs = $wfNode->attributes;
      //foreach ($tmpAttrs as $attr) 
      //{
        //$result['attributes'][$attr->name] = $attr->value;
      //}
    }

    $parents = $fedora_object->getParents();
    $collectionArray = null;
    foreach ($parents as $collectionId)
    {
      $collectionArray[] = array (
        'object_id' => $pid, 
        'member_collection_id' => $collectionId
      );
    }

    islandora_plotit_cwrc_viz_update_delta($fedora_object, $wfDocArray);
    islandora_plotit_cwrc_viz_update_datastore($pid, $wfDocArray);
    islandora_plotit_cwrc_viz_update_collectionstore($pid, $collectionArray);
  }
}

/**
 * add delta calculation to data structure 
 * assumes sorted input
 *
 */
function islandora_plotit_cwrc_viz_update_delta($fedora_object, &$wfDocArray)
{
  /*
  for ($i=0; $i < count($wfDocArray); $i++)
  {
    $wfDocArray[$i]['delta_by_size'] = rand(0,10000);
  }
   */

  $datastream = null;
  
  if ($fedora_object) 
  {
    // get the datastream to use for the size comparison
    foreach ($fedora_object->models as $cModel)
    {
      if ($cModel == 'cwrc:documentCModel') 
      {
        $datastream = $fedora_object['CWRC'];
        break;
      }
      else if ($cModel == 'cwrc:citationCMode') {
        $datastream = $fedora_object['MODS'];
        break;
      }
      else if ($cModel == 'cwrc:person-entityCModel') {
        $datastream = $fedora_object['PERSON'];
        break;
      }
      else if ($cModel == 'cwrc:organization-entityCModel') {
        $datastream = $fedora_object['ORGANIZATION'];
        break;
      }
      else {
        $datastream = $fedora_object['MODS'];
        break;
      }
    }
  }

    //var_dump($datastream->content);
    //var_dump($datastream->createdDate);

  //print("asdf" . $verIter);
  //if ($verIter == 1) print_r($datastream[1]->createdDate);
  //if ($verIter == 1) print_r($datastream[1]->size);


  $wfIter = count($wfDocArray) - 1;
  $verIter = 0;
  
  // assume sorted by date/time
  // traverse both arrays matching workflow activities with Fedora datastream
  // versions

 //var_dump($wfDocArray); 

  // ToDo: time zone - maybe change workflow schema to include timezone info? 
  do {
    //var_dump($datastream[$verIter]->size);

    $wfDate = $wfDocArray[$wfIter]['wf_date'];
    $verDate = $datastream[$verIter]->createdDate;


    if ($wfIter <= 0 and $verIter < count($datastream))
    {
      //ran out of the wf items so accumulate on first workflow activity
      $deltaSize = islandora_plotit_cwrc_viz_delta_versions_by_size($datastream,$verIter);
      //$wfDocArray[0]['delta_by_size'] = $wfDocArray[0]['delta_by_size'] + $datastream[$verIter]->size; 
      $wfDocArray[0]['delta_by_size'] = $wfDocArray[0]['delta_by_size'] + $deltaSize; 
      $verIter++;
    }
    else if ($verIter >= count($datastream) and $wfIter >= 0)
    {
      // ran out of versions therefor delta size is zero
      //$wfDocArray[$wfIter]['delta_by_size'] = 0; 
      $wfIter--;
    }
    else if (strtotime($wfDate) >= strtotime($verDate) and $wfIter >  0)
    {
      //don't use last workflow activity in this condition
      // there is another workflow activity closer to ds version's date/time  
      $nextWFDate =  $wfDocArray[$wfIter-1]['wf_date'];
      if (strtotime($nextWFDate) < strtotime($verDate))
      {
        $deltaSize = islandora_ploti_cwrc_viz_delta_versions_by_size($datastream,$verIter);
        //$wfDocArray[$wfIter]['delta_by_size'] = $wfDocArray[$wfIter]['delta_by_size'] + $datastream[$verIter]->size; 
        $wfDocArray[$wfIter]['delta_by_size'] = $wfDocArray[$wfIter]['delta_by_size'] + $deltaSize; 
        $verIter++;
        $wfIter--;
      }
      else
      {
        $wfDocArray[$wfIter]['delta_by_size'] = 0; 
        $wfIter--;
      }
      //print("a:".$wfDate."b:".$verDate."\n");
      //print("a:".strtotime($wfDate)."b:".strtotime($verDate)."\n");
    }
    else if ($wfDate < $verDate and $wfIter > 0)
    {
      $verIter++;
    }
    else
    {
      // should never happen
      $wfIter--;
      $verIter++;
    }
    
   // print("a:".$wfIter."b:".$verIter."\n");

  } while ($wfIter >= 0 or $verIter < count($datastream));


  //var_dump($wfDocArray);

  /*
  foreach ($wfDocArray as $item)
  {
    $item['delta_by_size'] = rand(0, 10000);
    print_r($item);
    print_r($wfDocArray);
  }
   */
}

/**
 * delta current and previous version
 */
function islandora_plotit_cwrc_viz_delta_versions_by_size($datastream, $currentVersion)
{
  $delta = 0;

  if ($currentVersion+1 < count($datastream))
  {
    $delta = abs($datastream[$currentVersion]->size - $datastream[$currentVersion+1]->size);
  } 
  else if ($currentVersion == count($datastream)-1)
  {
    $delta = $datastream[$currentVersion]->size;
  }
  else
  {
    print('Error in calculating delta');
  }

  return $delta;
}


/**
 * add workflow content to the datastore
 */
function islandora_plotit_cwrc_viz_update_datastore($pid, $wfDocArray)
{

  // ToDo: 2016-11-09 only add new workflow instead of the crude delete
  // Build Workflow metadata store for efficient access
  $query = db_delete(ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE)->condition('object_pid', $pid)->execute();

  $fields = array(
    'workflow_id'
    , 'object_pid'
    , 'object_label'
    , 'user_id'
    , 'user_name'
    , 'user_uri'
    , 'wf_date'
    , 'wf_time'
    , 'wf_category'
    , 'wf_stamp'
    , 'wf_status'
    , 'delta_by_size'

  );
  $query = db_insert(ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE)->fields($fields);
  foreach ($wfDocArray as $item)
  {
    $query->values($item);
  }
  $query->execute();
}


/**
 * add workflow content to the collection store
 */
function islandora_plotit_cwrc_viz_update_collectionstore($pid, $collectionArray)
{
  // ToDo: 2016-11-09 only add new workflow instead of the crude delete
  // Collection store as an object may belong to multiple collections
  $query = db_delete(ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE)->condition('object_pid', $pid)->execute();

  $fields = array(
    'object_pid'
    ,'member_collection_pid'
  );
  $query = db_insert(ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE)->fields($fields);
  $query->values($collectionArray);
  $query->execute();
}

