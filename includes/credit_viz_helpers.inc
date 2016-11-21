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
function islandora_cwrc_credit_visualization_api()
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
    $query_result = db_query("SELECT * FROM {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} WHERE object_pid IN (:pids) ORDER BY object_pid, wf_timestamp, workflow_id ",
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
      . " WHERE cs.object_pid=ds.object_pid and cs.member_collection_pid IN (:pids) ORDER BY object_pid, wf_timestamp, workflow_id ",
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
    $query_result = db_query("SELECT * FROM {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} WHERE object_pid IN (SELECT object_pid FROM  {".ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE."} WHERE user_id IN (:user_ids) ) ORDER BY object_pid, wf_timestamp, workflow_id ",
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
        $modArray['diff_changes'] = array($row['delta_by_size']);

        $detArray['modifications'][] = $modArray;
      }

    } while ($row);
  }

  $ret['documents'] = $docArray;

  drupal_json_output($ret);
}

/**
 * process the workflow datastream for one doc 
 */
function islandora_cwrc_viz_one_PID($pid)
{

  $fedora_object = islandora_object_load($pid);

  $wfDS = $fedora_object[ISLANDORA_CWRC_CREDIT_VIZ_WORKFLOW];

  if ($wfDS->content)
  {
    $wfObj = new DOMDocument();
    $wfObj->loadXML($wfDS->content);
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
      // fedora object properties
      $wfTmpArray['object_pid'] = $fedora_object->id;
      $wfTmpArray['object_label'] = $fedora_object->label;
      //user id
      $wfTmpArray['user_id'] = $wfNode->getAttribute("userID"); 
      // get users human readible name
      $tmpUser = user_load_by_name($wfNode->getAttribute("userID"));
      if (module_exists('realname') and $tmpUser)
      {
        $wfTmpArray['user_name'] = $tmpUser->realname; 
      }
      else
      {
        $wfTmpArray['user_name'] = $wfNode->getAttribute("userID"); 
      }
      
      $wfTmpArray['user_uri'] =  '';

      // ToDo: what timezone is used in legacy data/times
      //$wfTmpArray['wf_time'] = $wfNode->getAttribute("time"); 
      //$wfTmpArray['wf_date'] = $wfNode->getAttribute("date");
      //if ($wfTmpArray['wf_time'])
      //{
      //  $wfTmpArray['wf_date'] .= 'T'.$wfTmpArray['wf_time'];
      //}
      //$tmpT = new DateTime($wfTmpArray['wf_date'], new DateTimeZone('UTC') );
      //$wfTmpArray['wf_date'] = $tmpT->format('c');

      $tmpDateTime = $wfNode->getAttribute("timestamp");
      if ($tmpDateTime)
      {
        // newstyle workflow circa 2016-12-01
        $wfTmpArray['wf_timestamp'] = $tmpDateTime;
      }
      else
      {
        // ToDo: what timezone is used
        // handle old legacy workflow without the timezone and precision
        // likely before 2016-12-01
        $tmpLegacyDate = $wfNode->getAttribute("date"); 
        $tmpLegacyTime = $wfNode->getAttribute("time"); 
        if ($tmpLegacyTime)
        {
          $tmpLegacyDate .= 'T'.$tmpLegacyTime;
        }
        // get legacy date/time in the default, hopefully correct, timezone
        $tmpDateObj = new DateTime($tmpLegacyDate, new DateTimeZone(date_default_timezone_get()) );
        // convert to UTC
        $tmpDateObj->setTimezone(new DateTimeZone('UTC'));
        $wfTmpArray['wf_timestamp'] = $tmpDateObj->format('c');
      }


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
    if (count($wfDocArray)>0)
    {
      islandora_cwrc_viz_update_delta($fedora_object, $wfDocArray);
    }
    islandora_cwrc_viz_update_datastore($pid, $wfDocArray);
    islandora_cwrc_viz_update_collectionstore($pid, $collectionArray);
  }
}

/**
 * add delta calculation to data structure 
 * assumes sorted input
 *
 */
function islandora_cwrc_viz_update_delta($fedora_object, &$wfDocArray)
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

  $wfIter = count($wfDocArray) - 1;
  $verIter = 0;
  
  // assume sorted by date/time
  // traverse both arrays matching workflow activities with Fedora datastream
  // versions

  // ToDo: time zone - maybe change workflow schema to include timezone info? 
  do {
    //var_dump($datastream[$verIter]->size);

    $wfDate = $wfDocArray[$wfIter]['wf_timestamp'];
    $verDate = $datastream[$verIter]->createdDate;

    if ($wfIter <= 0 and $verIter < count($datastream))
    {
      //ran out of the wf items so accumulate on first workflow activity
      $deltaSize = islandora_cwrc_viz_delta_versions_by_size($datastream,$verIter);
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
      $nextWFDate =  $wfDocArray[$wfIter-1]['wf_timestamp'];
      if (strtotime($nextWFDate) < strtotime($verDate))
      {
        $deltaSize = islandora_cwrc_viz_delta_versions_by_size($datastream,$verIter);
        $wfDocArray[$wfIter]['delta_by_size'] = $wfDocArray[$wfIter]['delta_by_size'] + $deltaSize; 
        $verIter++;
        //$wfIter--; // might have workflow with before version of datastream
      }
      else
      {
        $wfDocArray[$wfIter]['delta_by_size'] = 0; 
        $wfIter--;
      }
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

  } while ($wfIter >= 0 or $verIter < count($datastream));

}

/**
 * delta current and previous version
 */
function islandora_cwrc_viz_delta_versions_by_size($datastream, $currentVersion)
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
function islandora_cwrc_viz_update_datastore($pid, $wfDocArray)
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
    , 'wf_timestamp'
    , 'wf_category'
    , 'wf_stamp'
    , 'wf_status'
    , 'delta_by_size'

  );

  if (count($wfDocArray)>0)
  {
    $query = db_insert(ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE)->fields($fields);
    foreach ($wfDocArray as $item)
    {
      if ($item['wf_category'])
      {
        $query->values($item);
      }
      else 
      {
        print('invalid workflow: ' . $item['object_pid']);
      }
    }
    $query->execute();
  }
}


/**
 * add workflow content to the collection store
 */
function islandora_cwrc_viz_update_collectionstore($pid, $collectionArray)
{
  // ToDo: 2016-11-09 only add new workflow instead of the crude delete
  // Collection store as an object may belong to multiple collections
  $query = db_delete(ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE)->condition('object_pid', $pid)->execute();

  $fields = array(
    'object_pid'
    ,'member_collection_pid'
  );
  if (count($wfDocArray)>0)
  {
    $query = db_insert(ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE)->fields($fields);
    $query->values($collectionArray);
    $query->execute();
  }
}

