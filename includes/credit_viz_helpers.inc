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
 * Prepare the JSON output for the credit visualization tool.
 */
function islandora_cwrc_credit_visualization_api() {
  $req = $_REQUEST;
  $ret = NULL;
  $query_result = NULL;
  $doc_array = array();

  if (array_key_exists(ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID, $req)) {
    $tmp = $req[ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID];
    $pid_array = NULL;
    if (is_array($tmp)) {
      $pid_array = $tmp;
    }
    else {
      $pid_array = array($tmp);
    }

    $ret['query'] = array(ISLANDORA_CWRC_CREDIT_VIZ_REQ_PID => $pid_array);

    // All workflow from the specified documents the specified collection needs
    // sorting for version difference calculation.
    $query_result = db_query("SELECT * FROM {" . ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE . "} WHERE object_pid IN (:pids) ORDER BY object_pid, wf_timestamp, workflow_id ",
      array(':pids' => $pid_array)
    );
  }
  elseif (array_key_exists(ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION, $req)) {
    $tmp = $req[ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION];
    $pid_array = NULL;
    if (is_array($tmp)) {
      $pid_array = $tmp;
    }
    else {
      $pid_array = array($tmp);
    }

    $ret['query'] = array(ISLANDORA_CWRC_CREDIT_VIZ_REQ_COLLECTION => $pid_array);

    // All workflow from documents at are members of the specified collection
    // needs sorting for version difference calculation.
    $query_result = db_query(
      "SELECT ds.* "
      . "FROM {". ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE ."} ds, {" . ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE ."} cs "
      . " WHERE cs.object_pid=ds.object_pid and cs.member_collection_pid IN (:pids) ORDER BY object_pid, wf_timestamp, workflow_id ",
      array(':pids' => $pid_array)
    );
  }
  elseif (array_key_exists(ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER, $req)) {
    $tmp = $req[ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER];
    $tmpArray = NULL;
    if (is_array($tmp)) {
      $tmpArray = $tmp;
    }
    else {
      $tmpArray = array($tmp);
    }

    $ret['query'] = array(ISLANDORA_CWRC_CREDIT_VIZ_REQ_USER => $tmpArray);

    // All workflow from documents touched by the specified users needs sorting
    // for version difference calculation.
    $query_result = db_query("SELECT * FROM {" . ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE . "} WHERE object_pid IN (SELECT object_pid FROM  {" . ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE . "} WHERE user_id IN (:user_ids) ) ORDER BY object_pid, wf_timestamp, workflow_id ",
      array(':user_ids' => $tmpArray)
    );
  }

  if ($query_result) {
    $last_pid = NULL;
    $det_array = NULL;
    $mod_array = NULL;
    $cnt = 0;

    do {
      $row = $query_result->fetchAssoc();
      if (!$row || ($row && $last_pid && $last_pid != $row['object_pid'])) {
        $doc_array[] = $det_array;
        $cnt = 0;
      }

      if ($row) {
        if ($last_pid == NULL || $last_pid != $row['object_pid']) {
          $det_array = NULL;
          $mod_array = NULL;
          $det_array['id'] = $row['object_pid'];
          $det_array['name'] = $row['object_label'];
          $last_pid = $row['object_pid'];
        }

        $mod_array['user']['id'] = $row['user_id'];
        $mod_array['user']['name'] = $row['user_name'];
        $mod_array['workflow_changes']['category'] = $row['wf_category'];
        $mod_array['workflow_changes']['stamp'] = $row['wf_stamp'];
        $mod_array['workflow_changes']['status'] = $row['wf_status'];
        $mod_array['workflow_changes']['timestamp'] = $row['wf_timestamp'];
        $mod_array['diff_changes'] = array($row['delta_by_size']);

        $det_array['modifications'][] = $mod_array;
      }

    } while ($row);
  }

  $ret['documents'] = $doc_array;

  drupal_json_output($ret);
}

/**
 * Process the workflow datastream for one doc.
 */
function islandora_cwrc_viz_one_pid($pid) {

  $fedora_object = islandora_object_load($pid);
  $wfDS = $fedora_object[ISLANDORA_CWRC_CREDIT_VIZ_WORKFLOW];

  if ($wfDS->content) {
    $wfObj = new DOMDocument();
    $wfObj->loadXML($wfDS->content);
    $xpath = new DOMXPath($wfObj);
    //$xpath->registerNamespace("php", "http://php.net/xpath");
    //$xpath->registerPhpFunctions();
    $wfNodeArray = $xpath->query('/cwrc/workflow');

    $wf_doc_array = array();
    $cnt = 0;
    foreach ($wfNodeArray as $wfNode) {

      $wfID = ($wfNode->getAttribute("workflowID"));
      if (!$wfID) {
        // If no workflow id.
        $wfID = $pid . "_wk_" . $cnt;
      }

      $wfTmpArray['workflow_id'] = $wfID;
      // Fedora object properties.
      $wfTmpArray['object_pid'] = $fedora_object->id;
      $wfTmpArray['object_label'] = $fedora_object->label;
      // User id.
      $wfTmpArray['user_id'] = $wfNode->getAttribute("userID");
      // Get users human readible name.
      $tmpUser = user_load_by_name($wfNode->getAttribute("userID"));
      if (module_exists('realname') and $tmpUser) {
        $wfTmpArray['user_name'] = $tmpUser->realname;
      }
      else {
        $wfTmpArray['user_name'] = $wfNode->getAttribute("userID");
      }

      $wfTmpArray['user_uri'] = '';

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
      if ($tmpDateTime) {
        // Newstyle workflow circa 2016-12-01.
        $wfTmpArray['wf_timestamp'] = $tmpDateTime;
      }
      else {
        // ToDo: what timezone is used
        // handle old legacy workflow without the timezone and precision likely
        // before 2016-12-01.
        $tmp_legacy_date = $wfNode->getAttribute("date");
        $tmpLegacyTime = $wfNode->getAttribute("time");
        if ($tmpLegacyTime) {
          $tmp_legacy_date .= 'T' . $tmpLegacyTime;
        }
        // Get legacy date/time in the default, hopefully correct, timezone.
        $tmp_date_obj = NULL;
        try {
          if ($tm = $tmpUser->timezone) {
            $tmp_date_obj = new DateTime($tmp_legacy_date, new DateTimeZone($tm));
          }
          else {
            $tmp_date_obj = new DateTime($tmp_legacy_date, new DateTimeZone(date_default_timezone_get()));
          }
          // Convert to UTC.
          $tmp_date_obj->setTimezone(new DateTimeZone('UTC'));
        }
        catch (Exception $e) {
          print("PID: {$pid}\nLABEL: {$fedora_object->label}\nError:\n{$e->getMessage()}\n");
          continue;
        }
        $wfTmpArray['wf_timestamp'] = $tmp_date_obj->format('c');
      }


      // Should only have one activity node.
      $wfActivityNode = $xpath->query('activity', $wfNode)->item(0);
      if ($wfActivityNode) {
        $wfTmpArray['wf_category'] = $wfActivityNode->getAttribute("category");
        $wfTmpArray['wf_stamp'] = $wfActivityNode->getAttribute("stamp");
        $wfTmpArray['wf_status'] = $wfActivityNode->getAttribute("status");
      }

      $wfTmpArray['delta_by_size'] = 0;
      $wf_doc_array[] = $wfTmpArray;
      $cnt++;

      //$tmpAttrs = $wfNode->attributes;
      //foreach ($tmpAttrs as $attr) 
      //{
        //$result['attributes'][$attr->name] = $attr->value;
      //}
    }

    // Update collection information.
    $parents = $fedora_object->getParents();
    $collection_array = NULL;
    foreach ($parents as $collection_id) {
      $collection_array[] = array(
        'object_pid' => $pid,
        'member_collection_pid' => $collection_id,
      );
    }

    // Get the version delta information.
    if (count($wf_doc_array) > 0) {
      islandora_cwrc_viz_update_delta($fedora_object, $wf_doc_array);
    }

    // Write to database.
    islandora_cwrc_viz_update_datastore($pid, $wf_doc_array);
    islandora_cwrc_viz_update_collectionstore($pid, $collection_array);
  }
}

/**
 * Add delta calculation to data structure assumes sorted input.
 */
function islandora_cwrc_viz_update_delta($fedora_object, &$wf_doc_array) {
  /*
  for ($i=0; $i < count($wf_doc_array); $i++)
  {
    $wf_doc_array[$i]['delta_by_size'] = rand(0,10000);
  }
   */

  $datastream = NULL;

  if ($fedora_object) {
    // Get the datastream to use for the size comparison.
    foreach ($fedora_object->models as $cModel) {
      if ($cModel == 'cwrc:documentCModel') {
        $datastream = $fedora_object['CWRC'];
        break;
      }
      elseif ($cModel == 'cwrc:citationCMode') {
        $datastream = $fedora_object['MODS'];
        break;
      }
      elseif ($cModel == 'cwrc:person-entityCModel') {
        $datastream = $fedora_object['PERSON'];
        break;
      }
      elseif ($cModel == 'cwrc:organization-entityCModel') {
        $datastream = $fedora_object['ORGANIZATION'];
        break;
      }
      else {
        $datastream = $fedora_object['MODS'];
        break;
      }
    }
  }

  $wfIter = count($wf_doc_array) - 1;
  $verIter = 0;

  // Assume sorted by date/time traverse both arrays matching workflow
  // activities with Fedora datastream versions.
  // ToDo: time zone - maybe change workflow schema to include timezone info?
  do {
    $wfDate = $wf_doc_array[$wfIter]['wf_timestamp'];

    if ($wfIter <= 0 and $verIter < count($datastream)) {
      // Ran out of the wf items so accumulate on first workflow activity.
      $deltaSize = islandora_cwrc_viz_delta_versions_by_size($datastream, $verIter);
      $wf_doc_array[0]['delta_by_size'] = $wf_doc_array[0]['delta_by_size'] + $deltaSize;
      $verIter++;
    }
    elseif ($verIter >= count($datastream) and $wfIter >= 0) {
      // Ran out of versions therefor delta size is zero.
      //$wf_doc_array[$wfIter]['delta_by_size'] = 0;
      $wfIter--;
    }
    elseif ($wfIter > 0) {
      $verDate = $datastream[$verIter]->createdDate;
      if (strtotime($wfDate) >= strtotime($verDate) and $wfIter > 0) {
        // Don't use last workflow activity in this condition there is another
        // workflow activity closer to ds version's date/time.
        $nextWFDate = $wf_doc_array[$wfIter - 1]['wf_timestamp'];
        if (strtotime($nextWFDate) < strtotime($verDate)) {
          $deltaSize = islandora_cwrc_viz_delta_versions_by_size($datastream, $verIter);
          $wf_doc_array[$wfIter]['delta_by_size'] = $wf_doc_array[$wfIter]['delta_by_size'] + $deltaSize;
          $verIter++;
          //$wfIter--; // might have workflow with before version of datastream
        }
        else {
          $wfIter--;
        }
      }
      elseif ($wfDate < $verDate && $wfIter > 0) {
        $verIter++;
      }
    }
    else {
      // Should never happen.
      $wfIter--;
      $verIter++;
    }

    //drush_print("a " . $wfIter . ' ' . $verIter . ' ' . $wf_doc_array[$wfIter]['wf_timestamp'] . ' ' .  $wf_doc_array[$wfIter]['delta_by_size']);

  } while ($wfIter >= 0 or $verIter < count($datastream));
}

/**
 * Delta current and previous version.
 */
function islandora_cwrc_viz_delta_versions_by_size($datastream, $current_version) {
  $delta = 0;

  if ($current_version + 1 < count($datastream)) {
    $delta = abs($datastream[$current_version]->size - $datastream[$current_version + 1]->size);
  }
  elseif ($current_version == count($datastream) - 1) {
    $delta = $datastream[$current_version]->size;
  }
  else {
    print('Error in calculating delta');
  }

  return $delta;
}

/**
 * Add workflow content to the datastore.
 */
function islandora_cwrc_viz_update_datastore($pid, $wf_doc_array) {

  // ToDo: 2016-11-09 only add new workflow instead of the crude delete
  // Build Workflow metadata store for efficient access.
  $query = db_delete(ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE)->condition('object_pid', $pid)->execute();

  $fields = array(
    'workflow_id',
    'object_pid',
    'object_label',
    'user_id',
    'user_name',
    'user_uri',
    'wf_timestamp',
    'wf_category',
    'wf_stamp',
    'wf_status',
    'delta_by_size',
  );

  if (count($wf_doc_array) > 0) {
    $query = db_insert(ISLANDORA_CWRC_CREDIT_VIZ_DATASTORE)->fields($fields);
    foreach ($wf_doc_array as $item) {
      if ($item['wf_category']) {
        $query->values($item);
      }
      else {
        print('invalid workflow: ' . $item['object_pid']);
      }
    }
    $query->execute();
  }
}

/**
 * Add workflow content to the collection store.
 */
function islandora_cwrc_viz_update_collectionstore($pid, $collection_array) {
  // ToDo: 2016-11-09 only add new workflow instead of the crude delete
  // Collection store as an object may belong to multiple collections.
  $query = db_delete(ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE)->condition('object_pid', $pid)->execute();

  $fields = array(
    'object_pid',
    'member_collection_pid',
  );
  if (count($collection_array) > 0) {
    $query = db_insert(ISLANDORA_CWRC_CREDIT_VIZ_COLLECTSTORE)->fields($fields);
    foreach ($collection_array as $item) {
      if ($item['member_collection_pid']) {
        $query->values($item);
      }
      else {
        print('invalid collection: ' . $item['object_pid']);
      }
    }
    $query->execute();
  }
}

function islandora_cwrc_credit_visualization_update_datastore_by_collection_tree($collection_objects) {
  foreach ($collection_objects as $pid => $object) {
    if ($object['is_collection']) {
      islandora_cwrc_credit_visualization_update_datastore_by_collection_tree($object['children']);
    }
    else {
      islandora_cwrc_viz_one_pid($pid);
    }
  }
}
