<?php

/**
 * @file
 * Purpose: provide helper functions for the Voyant connector.
 *
 * Given either a collection PID or bookmark id, build a list
 * of PIDs and from the list build a zip archive of
 * Voyant interpretable content to send via multi-part from request.
 *
 * Todo: consider adding to https://github.com/cwrc/cwrc_visualization
 */

/**
 * Credit visualization highchart form.
 *
 * @param array $form
 *   The form.
 * @param array $form_state
 *   The form state.
 * @param \IslandoraFedoraObject $islandora_object
 *   The islandora object.
 *
 * @return array
 *   The form structure.
 *
 * @throws \Exception
 */
function islandora_cwrc_credit_visualization_highchart_form(array $form, array &$form_state, IslandoraFedoraObject $islandora_object) {
  $form['#islandora_object'] = $islandora_object;
  $module_name = 'islandora_cwrc_credit_visualization';
  $path = drupal_get_path('module', $module_name);

  // Getting url query parameters.
  $parameters = drupal_get_query_parameters();
  // Selected users.
  $selected_users = !empty($parameters['users']) ? $parameters['users'] : array();
  // Excluded datastore ids through documents pids.
  $datastore_param = !empty($parameters['excluded_datastore_ids']) ? $parameters['excluded_datastore_ids'] : '';
  $excluded_datastore_ids = explode(',', $datastore_param);
  // Date range.
  $date_range = !empty($parameters['date_range']) ? $parameters['date_range'] : array();
  if (!_islandora_cwrc_credit_visualization_is_valid_date($date_range)) {
    $date_range = array();
  }
  // Checking if filters were applied.
  $filtered = $selected_users && $date_range;

  $is_collection = _cwrc_islandora_bagit_object_is_collection_policy($islandora_object);
  // Updating filtered flag for collections.
  if ($is_collection && $datastore_param && !$filtered) {
    $filtered = TRUE;
  }
  $form['#islandora_object_is_collection'] = $is_collection;
  $query = db_select('islandora_cwrc_credit_viz_datastore', 'ds')
    ->fields('ds');
  $options = array();

  if ($is_collection) {
    $objects = islandora_cwrc_credit_visualization_get_collection_members_tree($islandora_object->id);
    $options = islandora_cwrc_credit_visualization_get_collection_tree_document_options($objects);
    $query->condition('ds.object_pid', array_keys($options), 'IN');
  }
  else {
    $query->condition('ds.object_pid', $islandora_object->id, '=');
  }
  $results = $query->orderBy('ds.wf_timestamp', 'ASC')
    ->execute();

  if (!$results) {
    $form['footer'] = array(
      '#markup' => t('<p>Contributions to <b>@label</b></p>', array(
        '@label' => $islandora_object->label,
      )),
    );
    return $form;
  }
  else {
    $users_contributions = array();
    $object_pids = array();
    $excluded_documents = array();
    $contribution_timestamps = array();
    $result_count = 0;
    $valid_range_min = FALSE;
    if (!empty($date_range['min'])) {
      $date_range_min = new \DateTime('now', new \DateTimeZone('UTC'));
      $date_range_max = clone $date_range_min;
      $date_range_min->setTimestamp($date_range['min']);
      $date_range_max->setTimestamp($date_range['max']);
      $date_range_min_formatted = $date_range_min->format('c');
      $date_range_max_formatted = $date_range_max->format('c');
    }
    else {
      $date_range_min_formatted = '';
      $date_range_max_formatted = '';
    }

    foreach ($results as $result) {
      $did = $result->did;
      $user_id = $result->user_id;
      $user_name = $result->user_name;
      $object_pid = $result->object_pid;
      $object_label = $result->object_label;
      $category = _islandora_cwrc_credit_visualization_possible_category_alias($result->wf_category);
      if ($category === 'machine_processed') {
        continue;
      }
      $wf_date = $result->wf_timestamp;
      $contribution_timestamps[] = $wf_date;
      if ($result_count === 0 && $date_range_min_formatted) {
        $valid_range_min = $date_range_min_formatted >= $wf_date;
      }

      // Contribution by users.
      $count = in_array($did, $excluded_datastore_ids) ? 0 : 1;
      if ($count && $valid_range_min) {
        $count = $wf_date >= $date_range_min_formatted && $wf_date <= $date_range_max_formatted ? 1 : 0;
      }
      if (!$count) {
        $excluded_documents[$object_pid] = $object_pid;
      }
      if (empty($users_contributions[$user_id])) {
        $users_contributions[$user_id] = array(
          'name' => $user_name,
          'count' => $count,
          'categories' => array(
            $category => array(
              'count' => $count,
              'documents' => array(
                $object_pid => array(
                  'label' => $object_label,
                  'count' => $count,
                ),
              ),
            ),
          ),
          'user_id' => $user_id,
        );
      }
      else {
        $users_contributions[$user_id]['count'] += $count;
        // Adding category count.
        if (empty($users_contributions[$user_id]['categories'][$category])) {
          $users_contributions[$user_id]['categories'][$category]['count'] = $count;
          $users_contributions[$user_id]['categories'][$category]['documents'] = array(
            $object_pid => array(
              'label' => $object_label,
              'count' => $count,
            ),
          );
        }
        else {
          $users_contributions[$user_id]['categories'][$category]['count'] += $count;
          if (empty($users_contributions[$user_id]['categories'][$category]['documents'][$object_pid])) {
            $users_contributions[$user_id]['categories'][$category]['documents'][$object_pid] = array(
              'label' => $object_label,
              'count' => $count,
            );
          }
          else {
            $users_contributions[$user_id]['categories'][$category]['documents'][$object_pid]['count'] += $count;
          }
        }
      }

      // Contribution by document.
      if (!isset($object_pids[$object_pid])) {
        $object_pids[$object_pid] = array(
          'count' => $count,
          'datastore_ids' => array($did),
          'selected' => (bool) $count,
        );
      }
      else {
        $object_pids[$object_pid]['count'] += $count;
        $object_pids[$object_pid]['datastore_ids'][] = $did;
      }
      $result_count++;
    }
  }

  // Sorting DESC user total contributions.
  uasort($users_contributions, static function ($usera, $userb) {
    if ($usera['count'] === $userb['count']) {
      return 0;
    }
    return ($usera['count'] > $userb['count']) ? -1 : 1;
  });
  // Extracting the user names.
  $user_names = array_map(static function ($user) {
    return $user['name'];
  }, $users_contributions);
  $user_ids = array_keys($user_names);
  $user_ids = array_combine($user_ids, $user_ids);

  // Remove users which are not selected.
  if ($selected_users) {
    $users_contributions = array_filter($users_contributions, static function ($contribution) use ($selected_users) {
      return isset($selected_users[$contribution['user_id']]);
    });
    // Making sure that the query only contains users.
    $selected_users = array_intersect_key($selected_users, $user_ids);
  }

  $wrapper_id = 'cwrc-islandora-credit-visualization-highchart-wrapper';
  $time_wrapper_id = 'cwrc-islandora-credit-visualization-highchart-as-of-time';
  // No ui settings.
  usort($contribution_timestamps, static function ($t1, $t2) {
    return $t1 - $t2;
  });
  $min = array_values($contribution_timestamps)[0];
  $max = array_values(array_slice($contribution_timestamps, -1))[0];
  $date_start = $valid_range_min ? $date_range_min_formatted : $min;
  $date_end = $valid_range_min ? $date_range_max_formatted : $max;
  $no_ui_slider = array(
    'placeholder_id' => 'cwrc-islandora-credit-visualization-highchart-filters-no-ui-date-range-slider',
    'range' => array(
      'min' => $min,
      'max' => $max,
    ),
    'start' => array(
      'begin' => $date_start,
      'end' => $date_end,
    ),
  );
  if ($is_collection && $options && $object_pids) {
    $options = array_intersect_key($options, $object_pids);
    $document_pids = array_keys($options);
    $document_pids = array_combine($document_pids, $document_pids);
    $document_pids = array_filter($document_pids, static function ($pid) use ($excluded_documents) {
      return !isset($excluded_documents[$pid]);
    });
  }
  $selected_users_count = $selected_users ? count($selected_users) : count($user_names);
  $total_users_count = count($user_names);

  $chart_title = t('<span style="font-weight:bold">Credit Visualization</span> of the <span style="font-weight:bold">@label</span> @type (as of <span id="@id">@time</span>)', array(
    '@label' => $islandora_object->label,
    '@type' => $is_collection ? 'collection' : 'document',
    '@id' => $time_wrapper_id,
    '@time' => gmdate('F d Y, H:i:s \G\M\T'),
  ));
  $header = array('', t('Unfiltered values'));
  $rows = array();
  $rows[0] = array(
    array('data' => t('Date range'), 'header' => TRUE),
    t('@start - @end', array(
      '@start' => date('m/d/Y', strtotime($min)),
      '@end' => date('m/d/Y', strtotime($max)),
    )),
  );
  $last_row_count = 1;
  if ($is_collection && $options && $object_pids) {
    $last_row_count = 2;
    $rows[1] = array(
      array('data' => t('Objects'), 'header' => TRUE),
      count($options),
    );
  }
  $rows[$last_row_count] = array(
    array('data' => t('Contributors'), 'header' => TRUE),
    $total_users_count,
  );
  $table_caption = '';
  if ($filtered) {
    $header[] = t('Applied Filters*');
    $rows[0][] = t('@start - @end', array(
      '@start' => date('m/d/Y', strtotime($date_start)),
      '@end' => date('m/d/Y', strtotime($date_end)),
    ));
    if ($is_collection && $options && $object_pids) {
      $rows[1][] = $document_pids ? count($document_pids) : count($options);
    }
    $rows[$last_row_count][] = $selected_users_count;
    $table_caption = t('<span style="font-size: 12px;color:#666666;fill:#666666;"><span style="font-weight: bold">*This visualization is filtered</span>. Non-filtered contributors appear only if they contributed to the objects and contribution categories being visualized within the specified time-frame.</span>');
  }

  $chart_title .=  theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'sticky' => FALSE,
    'attributes' => array('class' => array('table--chart-title')),
    'caption' => $table_caption,
  ));
  $chart = array(
    '#prefix' => '<div id="' . $wrapper_id . '">',
    '#type' => 'chart',
    '#chart_id' => 'object_credviz',
    '#title' => $chart_title,
    '#chart_type' => 'bar',
    '#chart_library' => 'highcharts',
    '#background' => '#FFFFFF',
    '#stacking' => TRUE,
    '#suffix' => '</div>',
    '#weight' => 3,
    '#height' => 600,
    '#users_contributions' => $users_contributions,
    '#is_collection' => $is_collection,
  );
  $chart['xaxis'] = array(
    '#type' => 'chart_xaxis',
    '#axis_type' => 'category',
    '#title' => t('contributor(s)'),
  );

  // Columns data by user and categories.
  $categories = _islandora_cwrc_credit_visualization_possible_categories();
  unset($categories['machine_processed']);
  foreach ($categories as $machine_name => $title) {
    if ($machine_name !== 'machine_processed') {
      $data = _islandora_cwrc_credit_visualization_get_chart_category_data($machine_name, $users_contributions, $is_collection);
      $chart[$machine_name] = [
        '#type' => 'chart_data',
        '#title' => $title,
        '#data' => $data,
      ];
    }
  }

  $filters_wrapper_id = 'cwrc-islandora-credit-visualization-highchart-filters-wrappers';
  $js_settings[$module_name] = array(
    'workflow_contributions' => $users_contributions,
    'workflow_contributors' => $user_names,
    'workflow_selected_contributors_count' => $selected_users_count,
    'workflow_contributors_count' => $total_users_count,
    'workflow_categories' => $categories,
    'time_wrapper_id' => $time_wrapper_id,
    'chart_title' => $chart_title,
    'filters_wrapper_id' => $filters_wrapper_id,
    'no_ui_slider' => $no_ui_slider,
    'is_collection' => $is_collection,
    'islandora_object_label' => $islandora_object->label,
  );
  $chart['#attached'] = array(
    'js' => array(
      array('type' => 'setting', 'data' => $js_settings),
      $path . '/js/noUiSlider/js/nouislider.js',
      $path . '/js/wnumb/wNumb.js',
      $path . '/js/chart_date_handler.js',
      $path . '/js/checkboxes_handler.js',
    ),
    'css' => array(
      $path . '/js/noUiSlider/css/nouislider.css',
      $path . '/css/chart.css',
    ),
  );
  $form['chart'] = $chart;
  $form['filters'] = array(
    '#title' => t('Filter'),
    '#type' => 'fieldset',
    '#weight' => 2,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['filters']['wrappers'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => $filters_wrapper_id,
      'class' => array($is_collection ? 'is-collection' : 'is-object'),
    ),
  );
  $form['filters']['wrappers']['top_actions'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('form-top-actions'),
    ),
    'apply' => array(
      '#markup' => t('<a href="#" role="button" id="top-actions-apply-button" data-button="apply">Apply</a>'),
    ),
    'reset' => array(
      '#markup' => t('<a href="#" role="button" id="top-actions-reset-button" data-button="reset">Reset</a>'),
    ),
  );

  // Users filter.
  natcasesort($user_names);
  $form['filters']['wrappers']['users'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Contributor (<span id="edit-users-select-all-toggle-wrapper"></span>)'),
    '#options' => $user_names,
    '#weight' => 3,
    '#default_value' => $selected_users ? $selected_users : $user_ids,
  );

  // Date filter.
  $form['filters']['wrappers']['date_range'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('form-items-date-range'),
    ),
    '#tree' => TRUE,
    '#weight' => 2,
  );
  $form['filters']['wrappers']['date_range']['min'] = array(
    '#type' => 'hidden',
    '#default_value' => strtotime($no_ui_slider['start']['begin']),
  );
  $form['filters']['wrappers']['date_range']['max'] = array(
    '#type' => 'hidden',
    '#default_value' => strtotime($no_ui_slider['start']['end']),
  );
  $form['filters']['wrappers']['date_range']['no_ui_slider'] = array(
    '#prefix' => t('<label>Date Range (<span id="filter-date-range-start">@start</span> - <span id="filter-date-range-end">@end</span> )</label>', array(
      '@start' => date('n/j/Y', strtotime($no_ui_slider['start']['begin'])),
      '@end' => date('n/j/Y', strtotime($no_ui_slider['start']['end'])),
    )),
    '#markup' => '<div id="' . $no_ui_slider['placeholder_id'] . '"></div>',
  );

  if ($options && $object_pids) {
    $form['#object_pids'] = $object_pids;
    natcasesort($options);
    $form['filters']['wrappers']['document_pids'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Item (<span id="edit-document-pids-select-all-toggle-wrapper"></span>)'),
      '#options' => $options,
      '#default_value' => $document_pids ? $document_pids : array(),
      '#weight' => 1,
    );
  }

  if (!$is_collection) {
    $access = islandora_object_access(ISLANDORA_ADD_DS, $islandora_object);
    $form['update_workflow'] = array(
      '#type' => 'submit',
      '#value' => t('Refresh workflow stamps'),
      '#access' => $access,
      '#weight' => 1,
      '#submit' => array('islandora_cwrc_credit_visualization_highchart_form_refresh_workflow_stamps_submit'),
    );
  }

  $form['filters']['wrappers']['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['filters']['wrappers']['actions']['apply'] = array(
    '#type' => 'submit',
    '#value' => t('Apply'),
    '#weight' => 1,
  );
  $form['filters']['wrappers']['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
    '#weight' => 2,
  );
  return $form;
}

function islandora_cwrc_credit_visualization_highchart_form_ajax_callback(array $form, array $form_state) {
  return $form['chart'];
}

function islandora_cwrc_credit_visualization_highchart_form_refresh_workflow_stamps_submit(array $form, array &$form_state) {
  $islandora_object = $form['#islandora_object'];
  $is_collection = $form['#islandora_object_is_collection'];

  if (!$is_collection) {
    module_load_include('inc', 'islandora_cwrc_credit_visualization', 'includes/credit_viz_helpers');
    $pid = $islandora_object->id;
    islandora_cwrc_viz_one_pid($pid);
  }
}

function islandora_cwrc_credit_visualization_highchart_form_submit(array $form, array &$form_state) {
  if ($form_state['triggering_element']['#value'] === t('Apply')) {
    $values = $form_state['values'];

    $query = [];
    // Adding users filters.
    if (!empty($values['users'])) {
      $users = $values['users'];
      $users = array_filter($users);
      $query['users'] = $users;
    }
    // Adding document pids to the query.
    if (!empty($values['document_pids'])) {
      $object_pids = $form['#object_pids'];
      $excluded_datastore_ids = array();
      foreach ($values['document_pids'] as $pid => $selected) {
        if (!$selected) {
          $datastore_ids = $object_pids[$pid]['datastore_ids'];
          foreach ($datastore_ids as $id) {
            $excluded_datastore_ids[] = $id;
          }
        }
      }
      $query['excluded_datastore_ids'] = implode(',', $excluded_datastore_ids);
    }

    // Adding date filters.
    $query['date_range'] = $values['date_range'];

    if ($query) {
      $form_state['redirect'] = array(current_path(), array('query' => $query));
    }
  }
  else {
    $form_state['redirect'] = array(current_path());
  }
}

function _islandora_cwrc_credit_visualization_possible_category_alias($category) {
  if ($category === 'user-tagged') {
    $category = 'user_tagged';
  }
  elseif ($category === 'peer-reviewed/evaluated') {
    $category = 'peer_evaluated';
  }
  return $category;
}

function _islandora_cwrc_credit_visualization_get_chart_category_data($category_mn, $contributions, $is_collection) {
  $data = array();
  $i = 0;
  foreach ($contributions as $user_id => $contribution) {
    $contribution_count = isset($contribution['categories'][$category_mn]) ? $contribution['categories'][$category_mn]['count'] : 0;
      $data[] = array(
        'name' => $contribution['name'],
        'y' => $contribution_count,
        'x' => $i,
        'drilldown' => $is_collection && !empty($contribution_count),
        'category_id' => $category_mn,
        'user_id' => $user_id,
      );
      $i++;
  }

  return $data;
}

function _islandora_cwrc_credit_visualization_include_workflow_result($result, $selected_users, $selected_pids = array()) {
  $included = isset($result->object_pid, $result->user_id);
  if ($included && $selected_users && !isset($selected_users[$result->user_id])) {
    $included = FALSE;
  }
  return $included;
}

function _islandora_cwrc_credit_visualization_is_valid_date($date_range, $min_valid_timestamp = 946684800, $max_valid_timestamp = 'now') {
  $max_valid_timestamp = $max_valid_timestamp !== 'now' ? $max_valid_timestamp : time();
  $min = !empty($date_range['min']) && is_numeric($date_range['min']) ? $date_range['min'] : 0;
  $max = !empty($date_range['max']) && is_numeric($date_range['max']) ? $date_range['max'] : 0;
  return ($min >= $min_valid_timestamp && $max <= $max_valid_timestamp) && $min < $max;
}
