<?php
// $Id: og.admin.inc,v 1.15.4.2 2009/06/22 15:59:27 weitzman Exp $

/**
 * @file
 * Admin settings for the organic groups module.
 */

/**
 * Provide an overview of the administrator menu items.
 *
 * @param $node
 *   The group node.
 */
function og_group_admin_overview($obj_type, $oid) {
  og_set_breadcrumb($obj_type, $oid);
  // Imitate the menu_get_item() content, so we can pass it to
  // theme_admin_block_content().
  $content = array(
    'people' => array(
      'title' => t('People'),
      'description' => t('Manage the group members.'),
      'href' => "og/$obj_type/$oid/admin/people",
      'localized_options' => array(),
    ),
    'roles' => array(
      'title' => t('Roles'),
      'description' => t('Manage the group roles.'),
      'href' => "og/$obj_type/$oid/admin/people/roles",
      'localized_options' => array(),
    ),
    'permissions' => array(
      'title' => t('Permissions'),
      'description' => t('Manage the group permissions.'),
      'href' => "og/$obj_type/$oid/admin/people/permissions",
      'localized_options' => array(),
    ),
  );

  $form['system_compact_link'] = array('#markup' => theme('system_compact_link'));
  $form['admin_block_content'] = array('#markup' => theme('admin_block_content', array('content' => $content)));

  return $form;
}
/**
 * Form builder; OG user administration page.
 *
 * @ingroup forms
 * @see og_user_admin_account_validate()
 * @see og_user_admin_account_submit()
 */
function og_user_admin_account($form, $form_state, $obj_type, $oid) {
  og_set_breadcrumb($obj_type, $oid, array(l(t('Group'), "$obj_type/$oid/og")));
  $group = og_get_group($obj_type, $oid);
  $header = array(
    'username' => array('data' => t('Username')),
    'status' => array('data' => t('Status')),
    'roles' => array('data' => t('Roles')),
  );

  $form['group'] = array('#type' => 'value', '#value' => $group);
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $options = array();
  foreach (module_implements('og_user_operations') as $module) {
    $result = call_user_func($module . '_og_user_operations', array(), array('group' => $group));
    foreach ($result as $operation => $array) {
      $options[$operation] = $array['label'];
    }
  }

  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => 'unblock',
  );

  $options = array();
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $destination = drupal_get_destination();

  $status = array('blocked' => t('blocked'), 'pending' => t('pending'), 'active' => t('active'));
  $roles = array_flip(og_get_group_roles($group->gid));
  $accounts = array();
  // Get the group real object, so we can check the user ID of the object.
  $object = current(entity_load($obj_type, array($group->oid)));

  foreach (og_get_group_users($group->gid, array()) as $account) {
    // Don't allow editing of the group manager.
    if (empty($object->uid) || $account->uid != $object->uid) {
      $accounts[$account->uid] = '';
      $users_roles = array();
      foreach (og_get_user_roles($obj_type, $oid, $account->uid) as $rid) {
        $users_roles[] = $roles[$rid];
      }
      asort($users_roles);

      $options[$account->uid] = array(
        'username' => theme('username', array('account' => $account)),
        'status' =>  $status[$account->state],
        'roles' => theme('item_list', array('items' => $users_roles)),
      );

    }
  }
  $form['accounts'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => !empty($object->uid) ? t('No people available apart of the group manager.') : t('No people available.'),
  );
  $form['pager'] = array('#markup' => theme('pager', array('tags' => NULL)));

  return $form;
}


function og_user_admin_account_validate($form, &$form_state) {
  $form_state['values']['accounts'] = array_filter($form_state['values']['accounts']);
  if (count($form_state['values']['accounts']) == 0) {
    form_set_error('', t('No users selected.'));
  }
}

/**
 * Submit the user administration update form.
 */
function og_user_admin_account_submit($form, &$form_state) {
  $nid = $form_state['values']['nid'];

  $operations = module_invoke_all('og_user_operations', $form, $form_state + array('nid' => $nid));
  $operation = $operations[$form_state['values']['operation']];
  // Filter out unchecked accounts.
  $accounts = array_filter($form_state['values']['accounts']);

  if ($function = $operation['callback']) {
    // Add in callback arguments if present.
    if (isset($operation['callback arguments'])) {
      $args = array_merge(array($accounts), $operation['callback arguments']);
    }
    else {
      $args = array($accounts);
    }
    call_user_func_array($function, array('nid' => $nid) + $args);

    drupal_set_message(t('The update has been performed.'));
  }
}

/**
 * Menu callback: administer roles.
 *
 * @ingroup forms
 * @see og_user_admin_role_validate()
 * @see og_user_admin_role_submit()
 * @see theme_og_user_admin_new_role()
 */
function og_user_admin_role($form, $form_state, $obj_type, $oid) {
  if ($group = og_get_group($obj_type, $oid)) {
    $form['group'] = array('#type' => 'value', '#value' => $group);

    $rid = arg(7);
    if ($rid) {
      og_set_breadcrumb($obj_type, $oid, array(l(t('Group'), "$obj_type/$oid/og")), l(t('Roles'), "og/$obj_type/$oid/admin/people/roles"));

      $og_roles = og_get_group_default_roles($group->gid);
      if (in_array($rid, $og_roles)) {
        drupal_goto("og/$obj_type, $oid/admin/people/roles");
      }
      // Display the edit role form.
      $role = og_user_role_load($rid);
      $form['name'] = array(
        '#type' => 'textfield',
        '#title' => t('Role name'),
        '#default_value' => $role->name,
        '#size' => 30,
        '#required' => TRUE,
        '#maxlength' => 64,
        '#description' => t('The name for this role. Example: "moderator", "editorial board", "site architect".'),
      );
      $form['rid'] = array(
        '#type' => 'value',
        '#value' => $rid,
      );
      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save role'),
      );
      $form['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete role'),
      );
    }
    else {
      og_set_breadcrumb($obj_type, $oid, array(l(t('Group'), "$obj_type/$oid/og")));

      $form['name'] = array(
        '#type' => 'textfield',
        '#size' => 32,
        '#maxlength' => 64,
      );
      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Add role'),
      );
      $form['#submit'][] = 'og_user_admin_role_submit';
      $form['#validate'][] = 'og_user_admin_role_validate';
    }
    return $form;
  }
  else {
    // Not a group node.
    drupal_not_found();
    exit;
  }
}

function og_user_admin_role_validate($form, &$form_state) {
  if ($form_state['values']['name']) {
    $roles = og_user_roles($form_state['values']['group']->gid);
    if (in_array($form_state['values']['name'], $roles)) {
      form_set_error('name', t('The role name %name already exists. Please choose another role name.', array('%name' => $form_state['values']['name'])));
    }
  }
  else {
    form_set_error('name', t('You must specify a valid role name.'));
  }
}

function og_user_admin_role_submit($form, &$form_state) {
  $group = $form_state['values']['group'];
  $role->gid = $group->gid;
  $role->name = $form_state['values']['name'];
  if (!empty($form_state['values']['rid'])) {
    $role->rid= $form_state['values']['rid'];
  }
  if ($form_state['values']['op'] == t('Save role')) {
    og_user_role_save($role);
    drupal_set_message(t('The role has been renamed.'));
  }
  elseif ($form_state['values']['op'] == t('Delete role')) {
    og_user_role_delete($form_state['values']['rid']);
    drupal_set_message(t('The role has been deleted.'));
  }
  elseif ($form_state['values']['op'] == t('Add role')) {
    og_user_role_save($role);
    drupal_set_message(t('The role has been added.'));
  }
  $form_state['redirect'] = 'og/' . $group->obj_type . '/' . $group->oid . '/admin/people/roles';
  return;
}

/**
 * Theme the new role form.
 *
 * @ingroup themeable
 */
function theme_og_user_admin_new_role($variables) {
  $form = $variables['form'];
  $header = array(t('Name'), array('data' => t('Operations'), 'colspan' => 2));
  // The group object.
  $group = $form['group']['#value'];

  $og_roles = og_get_group_default_roles($group->gid);

  foreach (og_user_roles($group->gid) as $rid => $name) {
    $edit_permissions = l(t('edit permissions'), 'og/' . $group->obj_type .'/' . $group->oid . '/admin/people/permissions/' . $rid);
    if (!in_array($rid, $og_roles)) {
      $rows[] = array($name, l(t('edit role'), 'og/' . $group->obj_type .'/' . $group->oid . '/admin/people/roles/edit/' . $rid), $edit_permissions);
    }
    else {
      $rows[] = array($name, t('locked'), $edit_permissions);
    }
  }
  $rows[] = array(drupal_render($form['name']), array('data' => drupal_render($form['submit']), 'colspan' => 2));

  $output = drupal_render_children($form);
  $output .= theme('table', array('header' => $header, 'rows' => $rows));

  return $output;
}


/**
 * Menu callback: administer permissions.
 *
 * @ingroup forms
 * @see user_admin_permissions_submit()
 * @see theme_user_admin_permissions()
 */
function og_user_admin_permissions($form, $form_state, $obj_type = NULL, $oid = NULL, $rid = NULL) {
  // If no node object is provided then the node ID is 0, which means this
  // is the default permissions settings.
  $group = !empty($oid) ? og_get_group($obj_type, $oid) : array();
  $form['group'] = array('#type' => 'value', '#value' => $group);

  if (!empty($group)) {
    $gid =  $group->gid;
    og_set_breadcrumb($obj_type, $oid, array(l(t('Group'), "$obj_type/$oid/og")));
  }
  else {
    $gid = 0;
  }

  // Retrieve role names for columns.
  $role_names = og_user_roles($gid);
  if (!empty($rid)) {
    $role_names = array($rid => $role_names[$rid]);
  }
  // Fetch permissions for all roles or the one selected role.
  $role_permissions = og_user_role_permissions($role_names);

  if (empty($group)) {
    // Add the 'bulk update' checkbox to the user roles.
    $role_names += array(0 => t('bulk update'));
  }

  // Store $role_names for use when saving the data.
  $form['role_names'] = array(
    '#type' => 'value',
    '#value' => $role_names,
  );
  // Render role/permission overview:
  $options = array();
  $hide_descriptions = !system_admin_compact_mode();
  foreach (og_permissions_get() as $module => $permissions) {
    $form['permission'][] = array('#markup' => $module, '#id' => $module);
    foreach ($permissions as $perm => $perm_item) {
      $access = !empty($group) ? og_user_access($gid, 'show ' . $perm) : TRUE;
      if ($access) {
        $options[$perm] = '';
        $form['permission'][$perm] = array(
          '#type' => 'item',
          '#markup' => $perm_item['title'],
          '#description' => $hide_descriptions ? $perm_item['description'] : '',
        );
        foreach ($role_names as $rid => $name) {
          // Builds arrays for checked boxes for each role
          if (isset($role_permissions[$rid][$perm])) {
            $status[$rid][] = $perm;
          }
        }
      }
    }
  }

  // Have to build checkboxes here after checkbox arrays are built
  foreach ($role_names as $rid => $name) {
    $form['checkboxes'][$rid] = array('#type' => 'checkboxes', '#options' => $options, '#default_value' => isset($status[$rid]) ? $status[$rid] : array());
    $form['role_names'][$rid] = array('#markup' => $name, '#tree' => TRUE);
  }

  if (empty($group)) {
    $form['#theme'] = array('og_user_admin_permissions');
  }
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save permissions'), '#submit' => array('og_user_admin_permissions_submit'));
  $form['#after_build'] = array('og_user_admin_permissions_after_build');

  return $form;
}

/**
 * Helper function to disable the checkboxes according to their 'roles' state.
 *
 * Every permission can decdeclate to which roles it applies, as some don't make
 * sense if applied to anonymous and authenticated user (e.g. subscribe to group
 * should appear only to anonymous members).
 */
function og_user_admin_permissions_after_build($element) {
  // Authenticated roles.
  $gid = !empty($form['group']['#value']->gid) ? $form['group']['#value']->gid : 0;
  $auth_rids = array_flip(og_user_roles($gid));
  // Remove the anonymous member from the authenticated roles.
  $anon_rid = array_shift($auth_rids);

  foreach (og_permissions_get() as $module_perms) {
    foreach ($module_perms as $key => $perm) {
      $perm['roles'] = drupal_map_assoc($perm['roles']);

      if (empty($perm['roles']) || empty($perm['roles'][OG_ANONYMOUS_ROLE])) {
        $element['checkboxes'][$anon_rid][$key]['#attributes']['disabled'] = TRUE;
      }
      if (empty($perm['roles']) || empty($perm['roles'][OG_AUTHENTICATED_ROLE])) {
        foreach ($auth_rids as $auth_rid) {
          $element['checkboxes'][$auth_rid][$key]['#attributes']['disabled'] = TRUE;
        }
      }
    }
  }

  return $element;
}
/**
 * Save permissions selected on the administer permissions page.
 *
 * @see og_user_admin_permissions()
 */
function og_user_admin_permissions_submit($form, &$form_state) {
  foreach ($form_state['values']['role_names'] as $rid => $name) {
    if (!empty($rid)) {
      og_user_role_change_permissions($rid, $form_state['values'][$rid]);
    }
    else {
      // Bulk update existing groups.
      $roles = array(
        OG_ANONYMOUS_DEFAULT_RID => $form_state['values'][OG_ANONYMOUS_DEFAULT_RID],
        OG_AUTHENTICATED_DEFAULT_RID => $form_state['values'][OG_AUTHENTICATED_DEFAULT_RID],
        OG_ADMINISTRATOR_DEFAULT_RID => $form_state['values'][OG_ADMINISTRATOR_DEFAULT_RID],
      );
      $count = og_permissions_bulk_update($roles, $form_state['values'][$rid]);
      drupal_set_message(format_plural($count, 'Group was updated', '@count group were updated.', array('@count' =>$count)));

    }
  }

  drupal_set_message(t('The changes have been saved.'));
}


/**
 * Theme the administer permissions page.
 *
 * @ingroup themeable
 */
function theme_og_user_admin_permissions($variables) {
  $form = $variables['form'];
  $gid = !empty($form['group']['#value']->gid) ? $form['group']['#value']->gid : 0;
  $roles = og_user_roles($gid);

  if (empty($gid)) {
    // Add the 'bulk update' checkbox to the user roles.
    $roles += array(0 => t('bulk update'));
  }
  foreach (element_children($form['permission']) as $key) {
    $row = array();
    // Module name.
    if (is_numeric($key)) {
      $row[] = array('data' => drupal_render($form['permission'][$key]), 'class' => array('module'), 'id' => 'module-' . $form['permission'][$key]['#id'], 'colspan' => count($form['role_names']['#value']) + 1);
    }
    else {
      // Permission row.
      $row[] = array(
        'data' => drupal_render($form['permission'][$key]),
        'class' => array('permission'),
      );
      foreach (element_children($form['checkboxes']) as $rid) {
        $row[] = array('data' => drupal_render($form['checkboxes'][$rid][$key]), 'class' => array('checkbox'), 'title' => $roles[$rid] . ' : ' . t($key));
      }
    }
    $rows[] = $row;
  }
  $header[] = (t('Permission'));
  foreach (element_children($form['role_names']) as $rid) {
    $header[] = array('data' => drupal_render($form['role_names'][$rid]), 'class' => array('checkbox'));
  }
  $output = theme('system_compact_link');
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'permissions')));
  $output .= drupal_render_children($form);
  return $output;
}


/**
 * Groups permissions on default settings form.
 *
 * Allow site admin to set which permissions are available on a per group basis.
 */
function og_admin_settings() {
  $form['og_settings'] = _og_content_types();

  $form['og_settings']['group_details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Group details'),
    '#description' => t('The default behavior of new groups.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE
  );
  // Groups directory visibility
  $options = array(
    t("New groups don't appear in the groups directory. Administrators control the directory exclusively."),
    t('New groups always appear in the groups directory.'),
    t('Group creator chooses whether her group appears in the directory. Defaults to <em>in directory</em>.'),
    t('Group creator chooses whether her group appears in the directory. Defaults to <em>not in directory</em>.'),
  );
  $form['og_settings']['group_details']['og_visibility_directory'] = array(
    '#type' => 'radios',
    '#title' => t('Groups directory control'),
    '#default_value' => variable_get('og_visibility_directory', OG_DIRECTORY_CHOOSE_TRUE),
    '#description' => t('Site admins always see the checkbox for adding a group to the <em>groups directory</em>. Note that changing this setting has no effect on existing posts. Re-save those posts to acquire this new setting.'),
    '#options' => $options
  );

  // Groups registration visibility.
  $options = array(
    t("New groups don't appear on the registration form. Administrators control the form exclusively."),
    t('New groups always appear on the registration form.'),
    t('Group creator chooses whether her group appears on the registration form. Defaults to <em>on form</em>.'),
    t('Group creator chooses whether her group appears on the registration form. Defaults to <em>not on form</em>.'),
  );
  $form['og_settings']['group_details']['og_visibility_registration'] = array(
    '#type' => 'radios',
    '#title' => t('Registration form control'),
    '#default_value' => variable_get('og_visibility_registration', OG_REGISTRATION_CHOOSE_FALSE),
    '#description' => t('OG admins always see the checkbox for adding a group to the <em>registration form</em>. Note that changing this setting has no effect on existing posts. Re-save those posts to acquire this new setting.'),
    '#options' => $options
  );

  // Audience form element
  $form['og_settings']['group_details']['og_audience'] = array(
    '#type' => 'checkbox',
    '#title' => t('Audience form element'),
    '#default_value' => variable_get('og_audience', TRUE),
    '#description' => t('Show a form element for the user to choose the "Audience" of the content. This enables the member to place her post into multiple groups. If unchecked, simplify the user interface by omitting the audience form element and assume the user wants to post into the current group. This simplification only applies to new nodes, and not to edits of existing content. Group administrators always see the audience form element.')
  );

  // audience required
  $options = array(t('Optional'), t('Required'));
  $form['og_settings']['group_details']['og_audience_required'] = array(
    '#type' => 'radios',
    '#title' => t('Audience required'),
    '#default_value' => variable_get('og_audience_required', 0),
    '#options' => $options,
    '#description' => t('Do you require that all (non administrator) posts be affiliated with a group? Note that changing this setting will affect existing posts when they are edited.')
  );
  unset($options);


  return system_settings_form($form);
}

/**
 * Return a table element with the content types and their organic groups type.
 */
function _og_content_types() {
  $form = array();

  // Content types.
  $is_configured = count(og_get_node_type_by_og_type(array('group'))) && count(og_get_node_type_by_og_type(array('group post')));
  if (!$is_configured) {
    form_set_error('content_types_table', t('You must designate at least one content type to act as a group node and another as a group post. <a href="!create">Create new content type</a> if needed.', array('!create' => url('admin/content/types/add', array('query' => drupal_get_destination())))));
  }

  $form['content_types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Content types'),
    '#description' => t('You may <a href="@content-types-add">create new content types</a> as needed. Note that you may have multiple types acting as groups and multiple types are allowed as group posts.', array('@content-types-add' => url('admin/content/types/add', array('query' => drupal_get_destination())))),
    '#collapsible' => TRUE,
    '#collapsed' => $is_configured,
  );

  $og_types = og_get_node_type_by_og_type();
  $node_types = node_type_get_types();

  $header = array(t('Type'), t('Usage'), t('Operations'));
  $rows = array();

  foreach ($og_types as $type => $usage) {
    $type_url = str_replace('_', '-', $type);
    $rows[] = array(
      $node_types[$type]->name,
      $usage,
      l(t('Edit'), "admin/content/node-type/$type_url", array('query' => drupal_get_destination())),
    );
  }
  $form['content_types']['content_types_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows
  );

  return $form;

}
