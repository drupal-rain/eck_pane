<?php

use \Drupal\ko\ECK;
use \Drupal\ko\EntityInfo;

$plugin = array(
  'title' => t('ECK pane'),
  'content type' => 'eck_pane_content_type_content_type',
  'content types' => 'eck_pane_content_type_content_types',
  'render callback' => 'eck_pane_content_type_render',
);

// -----------------------------------------------------------------------------
// Subtype

/**
 * To create $subtype_name is eck_pane:<Entity Type>:<Bundle>.
 * Existing $subtype_name is <Entity Type>:<Id>
 */
function eck_pane_content_type_content_type($subtype_name) {
  $types = _eck_pane_content_type_content_types();
  if (isset($types[$subtype_name])) {
    return $types[$subtype_name];
  }
  else {
    list($entity_type, $id) = explode(':', $subtype_name);
    $entity = entity_load_single($entity_type, $id);
    if ($entity) {
      $entity_info = entity_get_info($entity_type);
      $title = isset($entity->title) ? $entity->title : $entity->id;
      $info = array(
        'title' => check_plain($title),
        'description' => check_plain($title),
        'category' => 'ECK: ' . $entity_info['label'],
        'all contexts' => TRUE,
        'edit form' => array('eck_pane_content_type_edit_form' => t('Edit')),
        'defaults' => array(
          'view_mode' => 'full',
        ),
      );

      return $info;
    }
  }
}

function eck_pane_content_type_content_types($plugin) {
  return _eck_pane_content_type_content_types();
}

function _eck_pane_content_type_content_types() {
  $types = array();

  $eck_entity_types = ECK::loadEntityTypes();

  foreach ($eck_entity_types as $entity_type => $bundles) {
    $entity_info = entity_get_info($entity_type);
    foreach ($bundles as $bundle) {
      $key = 'eck_pane:' . $entity_type . ':' . $bundle;
      $types[$key] = array(
        'title' => $entity_info['label'] . ': ' . $entity_info['bundles'][$bundle]['label'],
        'description' => $entity_info['label'] . ': ' . $entity_info['bundles'][$bundle]['label'],
        'category' => 'ECK: ' . $entity_info['label'],
        'add form' => array(
          'eck_pane_content_type_attach_form' => array(
            'default' => TRUE,
            'title' => t('Attach'),
          ),
          'eck_pane_content_type_add_form' => t('Add'),
        ),
        'defaults' => array(
          'view_mode' => 'full',
        ),
      );
    }
  }

  return $types;
}

// -----------------------------------------------------------------------------
// Attach Form

function eck_pane_content_type_attach_form($form, &$form_state) {

  unset($form['override_title']);
  unset($form['override_title_text']);
  unset($form['override_title_markup']);

  $form['entity_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Entity ID'),
    '#description' => t('Leave empty to create a new entity.'),
    '#default_value' => '',
  );

  return $form;
}

function eck_pane_content_type_attach_form_validate($form, &$form_state) {

}

function eck_pane_content_type_attach_form_submit($form, &$form_state) {
  $entity_id = $form_state['values']['entity_id'];
  if (!empty($entity_id)) {
    list(, $entity_type, $bundle) = explode(':', $form_state['subtype_name']);
    $entity = entity_load_single($entity_type, $entity_id);
    if ($entity) {
      // Update subtype info
      $subtype = $entity_type . ':' . $entity_id;
      // Skip to edit form
      $path = $form_state['form_info']['path'];
      $form_state['no_redirect'] = FALSE;
      $redirect = str_replace('%step', 'step1', $path);
      $redirect = str_replace($form_state['subtype_name'], $subtype, $redirect);
      $form_state['redirect'] = $redirect;
    }
  }
}

// -----------------------------------------------------------------------------
// Add Form

function eck_pane_content_type_add_form($form, &$form_state) {
  ctools_form_include_file($form_state, $form_state['plugin']['path'] . '/' . $form_state['plugin']['file']);

  $conf = &$form_state['conf'];

  list(, $entity_type, $bundle) = explode(':', $form_state['subtype_name']);
  $entity = entity_create($entity_type, array('type' => $bundle));
  $form_state['build_info']['args'] = array($entity);
  $form += drupal_retrieve_form("eck__entity__form_add_{$entity_type}_{$bundle}", $form_state);

  unset($form['override_title']);
  unset($form['override_title_text']);
  unset($form['override_title_markup']);
  unset($form['actions']);

  $form['view_mode'] = array(
    '#title' => t('View mode'),
    '#type' => 'select',
    '#description' => t('Select a view mode for this pane.'),
    '#options' => EntityInfo::getViewModesOptions($entity_type),
    '#default_value' => $conf['view_mode'],
    '#weight' => 100,
  );

  return $form;
}

function eck_pane_content_type_add_form_validate($form, &$form_state) {
  eck__entity__form_validate($form, $form_state);
}

function eck_pane_content_type_add_form_submit($form, &$form_state) {
  eck_pane_eck__entity__form_submit($form, $form_state);

  // Change the $subtype_name once the pane is created
  list(, $entity_type, $bundle) = explode(':', $form_state['subtype_name']);
  $entity = $form_state['values']['entity'];
  $subtype = $entity_type . ':' . $entity->id;
  $form_state['pane']->subtype = $subtype;

  // Save conf
  $form_state['conf']['view_mode'] = $form_state['values']['view_mode'];
}

// -----------------------------------------------------------------------------
// Edit Form

function eck_pane_content_type_edit_form($form, &$form_state) {
  ctools_form_include_file($form_state, $form_state['plugin']['path'] . '/' . $form_state['plugin']['file']);

  $conf = &$form_state['conf'];

  list($entity_type, $id) = explode(':', $form_state['subtype_name']);
  $entity = entity_load_single($entity_type, $id);
  $form_state['build_info']['args'] = array($entity);
  $form += drupal_retrieve_form("eck__entity__form_edit_{$entity_type}_{$entity->bundle()}", $form_state);

  unset($form['override_title']);
  unset($form['override_title_text']);
  unset($form['override_title_markup']);
  unset($form['actions']);

  $form['view_mode'] = array(
    '#title' => t('View mode'),
    '#type' => 'select',
    '#description' => t('Select a view mode for this pane.'),
    '#options' => EntityInfo::getViewModesOptions($entity_type),
    '#default_value' => $conf['view_mode'],
    '#weight' => 100,
  );

  return $form;
}

function eck_pane_content_type_edit_form_validate($form, &$form_state) {
  eck__entity__form_validate($form, $form_state);
}

function eck_pane_content_type_edit_form_submit($form, &$form_state) {
  eck_pane_eck__entity__form_submit($form, $form_state);

  // Save conf
  $form_state['conf']['view_mode'] = $form_state['values']['view_mode'];
}

/*
function eck_pane_content_type_edit_form($form, &$form_state) {
  dpm($form_state, '$form_state');
  return $form;
}
*/

function eck_pane_content_type_render($subtype, $conf, $args, $pane_context, $incoming_content) {
  $block = new stdClass();
  $block->title = t('');
  $entity = eck_pane_load_entity($subtype);
  $block->content = $entity->view($conf['view_mode']);

  return $block;
}
