<?php

/**
 * @file
 * Defines forms for managing objects with child pages.
 *
 * Management Forms:
 *  Create PDF.
 *  Create OCR.
 *  Create Images.
 *  Update Thumbnail.
 *  Reorder Pages.
 *  Delete Pages.
 */

require_once dirname(__FILE__) . '/utilities.inc';
require_once dirname(__FILE__) . '/batch.inc';

/**
 * Displays all the book management forms in a set of vertical tabs.
 *
 * @param AbstractObject $object
 *   The book object to manage.
 *
 * @return array
 *   A renderable array containing all the management forms related to
 *   book objects.
 */
function islandora_paged_content_manage_pages_menu(AbstractObject $object) {
  module_load_include('inc', 'islandora', 'includes/breadcrumb');
  drupal_set_breadcrumb(islandora_get_breadcrumbs($object));
  drupal_set_title($object->label);
  return array(
    'manage_pages' => array(
      '#type' => 'vertical_tabs',
      'pdf' => array(
        '#access' => user_access(ISLANDORA_ADD_DS),
        '#title' => t('Create PDF'),
        '#type' => 'fieldset',
        'form' => drupal_get_form('islandora_paged_content_manage_pages_pdf_form', $object),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
      ),
      'ocr' => array(
        '#access' => user_access(ISLANDORA_ADD_DS),
        '#title' => t('Perform OCR'),
        '#type' => 'fieldset',
        'form' => drupal_get_form('islandora_paged_content_manage_pages_ocr_form', $object),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      ),
      'images' => array(
        '#access' => user_access(ISLANDORA_ADD_DS),
        '#title' => t('Create Images'),
        '#type' => 'fieldset',
        'form_1' => drupal_get_form('islandora_paged_content_manage_pages_thumbnail_form', $object),
        'form_2' => drupal_get_form('islandora_paged_content_manage_pages_images_form', $object),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      ),
      'sequence' => array(
        '#access' => user_access(ISLANDORA_METADATA_EDIT),
        '#title' => t('Reorder Pages'),
        '#type' => 'fieldset',
        'form' => drupal_get_form('islandora_paged_content_manage_pages_sequences_form', $object),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      ),
      'delete' => array(
        '#access' => user_access(ISLANDORA_PURGE),
        '#title' => t('Delete Pages'),
        '#type' => 'fieldset',
        'form' => drupal_get_form('islandora_paged_content_manage_pages_delete_form', $object),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      ),
    ),
  );
}

/**
 * Creates a PDF of all the child pages.
 *
 * Generates a PDF file for each page and then combines them into a single PDF.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object to fetch the pages from.
 *
 * @return array
 *   The Drupal form.
 */
function islandora_paged_content_manage_pages_pdf_form(array $form, array &$form_state, AbstractObject $object) {
  $form_state['object'] = $object;
  $can_derive = islandora_paged_content_can_create_pdf() && islandora_paged_content_can_combine_pdf();
  return array(
    'description' => array(
      '#type' => 'item',
      '#description' => t('You must have both <b>ImageMagick</b> and <b>GhostScript</b> installed to create/combine PDF files.<br/> This will also update the PDF datastreams for each Page object.'),
    ),
    'dpi' => array(
      '#type' => 'select',
      '#title' => t('DPI - Dots Per Inch'),
      '#description' => t('Set the DPI for the generated PDF. The higher the resolution the longer the process will take.'),
      '#options' => drupal_map_assoc(array('72', '96', '300', '600')),
    ),
    'submit' => array(
      '#disabled' => !$can_derive,
      '#type' => 'submit',
      '#value' => t('Create PDF'),
    ),
  );
}

/**
 * Creates a PDF of the book and a PDF of every page.
 *
 * Triggers a batch to derive a PDF datastreams in each page object.then
 * combined them into a single PDF which is stored in the book object.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_manage_pages_pdf_form_submit(array $form, array &$form_state) {
  $object = $form_state['object'];
  $pages = array_keys(islandora_paged_content_get_pages($object));
  $options = array(
    '-density' => $form_state['values']['dpi'],
    '-compress' => 'LZW',
  );
  $batch = islandora_paged_content_create_pdf_batch($object, $pages, $options);
  batch_set($batch);
}

/**
 * Derives the OCR datastreams in each child page.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object to fetch the pages from.
 *
 * @return array
 *   The Drupal form.
 */
function islandora_paged_content_manage_pages_ocr_form(array $form, array &$form_state, AbstractObject $object) {
  $form_state['object'] = $object;
  $can_derive = FALSE;
  $languages = array();
  if (module_exists('islandora_ocr')) {
    module_load_include('inc', 'islandora_ocr', 'includes/utilities');
    $can_derive = islandora_ocr_can_derive_ocr();
    $languages = islandora_ocr_get_enabled_tesseract_languages();
  }
  return array(
    'description' => array(
      '#type' => 'item',
      '#description' => t('You must have <b>Tesseract</b> installed to perform OCR.<br/> This will update the OCR and HOCR datastreams for each Page object.'),
    ),
    'language' => array(
      '#access' => $can_derive,
      '#title' => t('Language'),
      '#type' => 'select',
      '#description' => t('Please select the language the pages are written in.'),
      '#options' => $languages,
    ),
    'submit' => array(
      '#disabled' => !$can_derive,
      '#type' => 'submit',
      '#value' => t('Create OCR'),
    ),
  );
}

/**
 * Triggers a batch to derive the OCR datastreams in each page object.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_manage_pages_ocr_form_submit(array $form, array &$form_state) {
  $object = $form_state['object'];
  $pages = array_keys(islandora_paged_content_get_pages($object));
  $options = array(
    'language' => $form_state['values']['language'],
    'preprocess' => FALSE,
  );
  $batch = islandora_paged_content_create_ocr_batch($object, $pages, $options);
  batch_set($batch);
}

/**
 * Updates the objects thumbnail from the first child page.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object to update.
 *
 * @return array
 *   The Drupal form.
 */
function islandora_paged_content_manage_pages_thumbnail_form(array $form, array &$form_state, AbstractObject $object) {
  $form_state['object'] = $object;
  return array(
    'description' => array(
      '#type' => 'item',
      '#description' => t("Update the thumbnail image. The book must have pages, and the first page must have a TN datastream."),
    ),
    'submit' => array(
      '#disabled' => !islandora_paged_content_can_update_paged_content_thumbnail($object),
      '#type' => 'submit',
      '#value' => t('Update Thumbnail'),
    ),
  );
}

/**
 * The submit handler for the update thumbnail form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_manage_pages_thumbnail_form_submit(array $form, array &$form_state) {
  if (islandora_paged_content_update_paged_content_thumbnail($form_state['object'])) {
    drupal_set_message(t('Thumbnail successfully updated.'), 'status');
  }
  else {
    drupal_set_message(t('Failed to update thumbnail'), 'error');
  }
}

/**
 * Derives the image datastreams for each page object.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object to fetch the pages from.
 *
 * @return array
 *   The Drupal form.
 */
function islandora_paged_content_manage_pages_images_form(array $form, array &$form_state, AbstractObject $object) {
  $form_state['object'] = $object;
  $can_derive = islandora_paged_content_can_create_images();
  return array(
    'description' => array(
      '#type' => 'item',
      '#description' => t('You must have the <b>Large Image Solution Pack</b> installed to create image derivatives.<br/> This will update the TN, JPG and JP2 datastreams for each page object.'),
    ),
    'submit' => array(
      '#disabled' => !$can_derive,
      '#type' => 'submit',
      '#value' => t('Create Images'),
    ),
  );
}

/**
 * Triggers a batch to derive image datastreams in each page object.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_manage_pages_images_form_submit(array $form, array &$form_state) {
  $object = $form_state['object'];
  $pages = array_keys(islandora_paged_content_get_pages($object));
  $batch = islandora_paged_content_create_images_batch($object, $pages);
  batch_set($batch);
}

/**
 * Form for reordering the child pages.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The book object to fetch the pages from.
 *
 * @return array
 *   The Drupal form.
 */
function islandora_paged_content_manage_pages_sequences_form(array $form, array &$form_state, AbstractObject $object) {
  $form_state['pid'] = $object->id;
  // Cache the generated rows, as this build function gets called for every
  // ajax callback.
  if (!isset($form_state['rows'])) {
    $pages = islandora_paged_content_get_pages($object);
    $rows = array();
    foreach ($pages as $page) {
      $id = $page['pid'];
      $url = "islandora/object/{$id}";
      $rows[$id] = array(
        'tn' => array(
          '#markup' => theme(
            'image', array(
              'path' => url("$url/datastream/TN/view"),
              'attributes' => array(
                'width' => '64px',
                'height' => '64px',
              )))),
        'label' => array(
          '#markup' => l($page['label'], $url, array(
                       'html' => TRUE,
                       'attributes' => array('title' => $page['label'])))),
      );
    }
    $form_state['rows'] = $rows;
  }
  return array(
    'table' => array(
      '#type' => 'swaptable',
      '#header' => array(
        t('Thumbnail'),
        t('Label'),
      ),
      '#rows' => $form_state['rows'],
    ),
    'quick_reorder' => array(
      '#type' => 'fieldset',
      '#title' => t('Quick Reorder'),
      'pages_to_move' => array(
        '#type' => 'textfield',
        '#title' => t('Move Pages:'),
        '#description' => t("Must follow the form '1, 2-4,5'."),
      ),
      'pages_insert_point' => array(
        '#type' => 'textfield',
        '#title' => t('Insert After:'),
      ),
      'submit' => array(
        '#validate' => array('islandora_paged_content_quick_reorder_validate'),
        '#type' => 'submit',
        '#submit' => array('islandora_paged_content_quick_reorder_submit'),
        '#value' => t('Go'),
      ),
    ),
    'actions' => array(
      '#type' => 'actions',
      'submit' => array(
        '#type' => 'submit',
        '#value' => t('Save Changes'),
      ),
    ),
  );
}

/**
 * Validates quick_reorder fieldset.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_quick_reorder_validate($form, &$form_state) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');

  $pages = islandora_paged_content_get_pages(islandora_object_load($form_state['pid']));
  $first_page = array_shift($pages);
  $last_page = array_pop($pages);
  $page_numbers = range($first_page['page'], $last_page['page']);

  // Insert point validation.
  if (!preg_match('/^[0-9\s]/', $form_state['values']['pages_insert_point']) ||
        !in_array($form_state['values']['pages_insert_point'], $page_numbers)) {
    form_set_error('pages_insert_point', t('Insert point must be a valid page number.'));
  }

  // Pages to move validation.
  if (!preg_match('/^[0-9\s,\-]/', $form_state['values']['pages_to_move'])) {
    form_set_error(
      'pages_to_move',
      t("Pages to move must follow the form '1, 2-4,5'.")
    );
  }
  $pages_to_move_list = islandora_paged_content_parse_pages_to_move($form_state['values']['pages_to_move']);
  if ($pages_to_move_list != array_intersect($pages_to_move_list, $page_numbers)) {
    form_set_error('pages_to_move', t('All pages must be in range.'));
  }
}

/**
 * Validates quick_reorder fieldset.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_quick_reorder_submit($form, &$form_state) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  $page_numbers_to_move = islandora_paged_content_parse_pages_to_move($form_state['values']['pages_to_move']);

  // Build page array without changed pids.
  $book = islandora_object_load($form_state['pid']);
  $raw_pages = islandora_paged_content_get_pages($book);
  $pages = array();
  $page_pids_to_move = array();
  foreach ($raw_pages as $page_pid => $page_info) {
    if (in_array($page_info['page'], $page_numbers_to_move)) {
      $page_pids_to_move[] = $page_pid;
    }
    else {
      $pages[$page_info['page']] = $page_pid;
    }
  }

  // Add in changed PIDs. Fixes sequence numbers.
  array_splice(
    $pages,
    array_search(
      $form_state['values']['pages_insert_point'],
      array_keys($pages)
    ) + 1,
    0,
    $page_pids_to_move
  );

  $batch = islandora_paged_content_sequence_pages_batch($book, $pages);
  batch_set($batch);
}

/**
 * Submit handler for the sequence form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_manage_pages_sequences_form_submit(array $form, array &$form_state) {
  $object = islandora_object_load($form_state['pid']);
  $pages = $form_state['values']['table']['order'];
  if ($pages) {
    $batch = islandora_paged_content_sequence_pages_batch($object, $pages);
    batch_set($batch);
  }
}

/**
 * Gets the delete pages form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object to fetch the pages from.
 *
 * @return array
 *   The Drupal form.
 */
function islandora_paged_content_manage_pages_delete_form(array $form, array &$form_state, AbstractObject $object) {
  $form_state['object'] = $object;
  $pages = islandora_paged_content_get_pages($object);
  return array(
    'table' => array(
      '#type' => 'tableselect',
      '#header' => array(
        'pid' => t('PID'),
        'page' => t('Sequence Number'),
        'label' => t('Label'),
      ),
      '#options' => $pages,
      '#multiple' => TRUE,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Delete Selected Pages'),
    ),
  );
}

/**
 * Submit handler for the delete pages form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_manage_pages_delete_form_submit(array $form, array &$form_state) {
  $object = $form_state['object'];
  $pages = array_values(array_filter($form_state['values']['table']));
  $batch = islandora_paged_content_delete_pages_batch($object, $pages);
  batch_set($batch);
}
