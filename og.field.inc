<?php
// $Id$

/**
 * @file
 * Field module functionality for the Organic groups module.
 */

/*******************************************************************************
 * Hook implementations
 ******************************************************************************/
/**
 * Implement hook_field_info().
 */
function og_field_info() {
  return array(
    'og' => array(
      'label' => t('Organic groups'),
      'description' => t('This field stores group nodes.'),
      'default_widget' => 'og_audience',
      'default_formatter' => 'og_list_default',
    ),
  );
}

/**
 * Implement hook_field_formatter_info().
 */
function og_field_formatter_info() {
  return array(
    'og_list_default' => array(
      'label' => t('OG default list'),
      'field types' => array('og'),
    ),
  );
}

/**
 * Implement hook_field_schema().
 */
function og_field_schema($field) {
  $columns = array(
    'value' => array(
      'type' => 'float',
      'unsigned' => TRUE,
      'not null' => FALSE,
    ),
    'data' => array(
      'description' => 'Serialized additional data about the group post.',
      'type' => 'text',
      'not null' => FALSE,
      'serialize' => TRUE,
    ),
    'state'     => array(
      'description'     => 'The state of the group post.',
      'type'            => 'varchar',
      'length'          => 255,
      'not null'        => FALSE,
      'default' => '',
    ),
  );
  return array(
    'columns' => $columns,
    'indexes' => array(
      'value' => array('value'),
    ),
  );
}

/**
 * Implement hook_field_widget_info().
 */
function og_field_widget_info() {
  return array(
    'og_audience' => array(
      'label' => t('Organic groups audience'),
      'field types' => array('og'),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM,
      ),
    ),
  );
}

/**
 * Implement hook_field_widget().
 */
function og_field_widget(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  // Select groups if they were passed in the URL, in the form of
  // node/add/post?gids[]=1,2,3
  $default_values = !empty($_REQUEST['gids'][0]) ? explode(',', $_REQUEST['gids'][0]) : array();

  if (!empty($items)) {
    foreach ($items as $item) {
      $default_values[$item['value']] = $item['value'];
    }
  }

  $element += array(
    '#type' => $instance['widget']['type'],
    '#default_value' => $default_values,
    // If it's an existing node, then exclude itself, as in some cases a group
    // node can act also as a group post, and we want to prevent associating
    // the node to itself.
    '#excludes' => !empty($form['nid']['#value']) ? array($form['nid']['#value']) : array(),
  );

  $defaults = array('data' => array(), 'state' => '');
  foreach ($items as $delta => $item) {
    $items[$delta] = array_merge($defaults, $items[$delta]);
  }
  return $element;
}

/**
 * Implement hook_field_is_empty().
 */
function og_field_is_empty($item, $field) {
  return empty($item['value']);
}

/**
 * Implement hook_field_insert().
 */
function og_field_insert($obj_type, $object, $field, $instance, $langcode, &$items) {
  og_field_write('insert', $obj_type, $object, $field, $instance, $langcode, $items);
}

/**
 * Implement hook_field_update().
 */
function og_field_update($obj_type, $object, $field, $instance, $langcode, &$items) {
  og_field_write('update', $obj_type, $object, $field, $instance, $langcode, $items);
}

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
/**
 * Insert or update a field record.
 *
 * @param $op
 *   The operation - "insert" or "update".
 */
function og_field_write($op, $obj_type, $object, $field, $instance, $langcode, &$items) {
  foreach ($items as &$item) {
    $item['state'] = !empty($item['state']) ? $item['state'] : OG_STATE_ACTIVE;
    $item['data'] = !empty($item['data']) ? $item['data'] : array();
    $alter = array('state' => $item['state'], 'data' => $item['data']);

    // Allow other modules to alter the data.
    drupal_alter('og_field_' . $op, $alter, $obj_type, $object, $field, $instance, $langcode, $items);
    $item['state'] = $alter['state'];
    $item['data'] = serialize($alter['data']);
  }
}


/**
 * Add organic groups audience field to fieldable entities that are group posts.
 *
 * @param $bundle
 *   The bundle name.
 * @param $obj_type
 *   The object type for the instance. "node" by default.
 */
function og_create_fields_group_posts($bundle, $obj_type = 'node') {
  $field = field_info_field('og_audience');
  if (empty($field)) {
    $field = array(
      'field_name' => 'og_audience',
      'type' => 'og',
      'cardinality' => FIELD_CARDINALITY_UNLIMITED,
    );
    $field = field_create_field($field);
  }

  $instance = field_info_instance($obj_type, 'og_audience', $bundle);
  if (empty($instance)) {
    $instance = array(
      'field_name' => 'og_audience',
      'bundle' => $bundle,
      'object_type' => $obj_type,
      'label' => t('Groups audience'),
      'widget_type' => 'og_audience',
      'display' => array(
        'full' => array(
          'label' => 'above',
          'type' => 'og_list_default',
        ),
        'teaser' => array(
          'label' => 'above',
          'type' => 'og_list_default',
        ),
      ),
    );
    field_create_instance($instance);
  }
}

/**
 * Add organic groups register field group types.
 *
 * @param $bundle
 *   The bundle name.
 */
function og_create_fields_group($bundle, $obj_type = 'node') {
  $field = field_info_field('og_group');
  if (empty($field)) {
    $field = array(
      'field_name' => 'og_group',
      'type' => 'list_boolean',
      'cardinality' => 1,
      'settings' => array('allowed_values' => implode("\r\n", array('0|Not a group type', '1|Group type')), 'allowed_values_function' => ''),
    );
    $field = field_create_field($field);
  }

  $instance = field_info_instance($obj_type, 'og_group', $bundle);
  if (empty($instance)) {
    $instance = array(
      'field_name' => 'og_group',
      'bundle' => $bundle,
      'object_type' => $obj_type,
      // FIXME: Widget type doesn't work
      'widget_type' => 'options_onoff',
      'label' => t('Group type'),
      'display' => array(
        'full' => array(
          'label' => 'above',
          'type' => 'options_onoff',
        ),
        'teaser' => array(
          'label' => 'above',
          'type' => 'options_onoff',
        ),
      ),
    );
    field_create_instance($instance);
  }
}

/*******************************************************************************
 * Theme functions
 ******************************************************************************/

/**
 * Organic group audience list field formatter.
 *
 * @return
 *   A link to the group node.
 */
function theme_field_formatter_og_list_default($variables) {
  $element = $variables['element'];
  $field = field_info_field($element['#field_name']);
  $options = og_allowed_values($field);
  // Flatten the options. We don't use array_merge() because we want to keep the
  // index key, which is the group node ID.
  $options = $options['content groups'] + $options['other groups'];
  // Return the group's link.
  $return = l($options[$element['#item']['value']], 'node/' . $element['#item']['value']);
  return $return;
}

