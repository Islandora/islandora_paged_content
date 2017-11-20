<?php

/**
 * @file
 * Handles the upload of a single page.
 */

/**
 * The form for uploading a single page image.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $parent
 *   An AbstractObject representing an object within Fedora.
 *
 * @return array
 *   An array containing the form to be rendered.
 */
function islandora_paged_content_upload_page_form(array $form, array &$form_state, AbstractObject $parent) {
  $get_default_value = function($name, $default) use(&$form_state) {
    return isset($form_state['values'][$name]) ? $form_state['values'][$name] : $default;
  };
  $form_state['parent'] = $parent;
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('tiff tif jp2 jpg jpeg');
  $languages = array();
  if (module_exists('islandora_ocr')) {
    module_load_include('inc', 'islandora_ocr', 'includes/utilities');
    $languages = islandora_ocr_get_enabled_tesseract_languages();
  }
  $default_language = in_array('eng', $languages) ? 'eng' : NULL;

  $upload_form = array(
    'file' => array(
      '#title' => t('Page'),
      '#type' => 'managed_file',
      '#description' => t('Select an image to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $extensions[0])),
      '#default_value' => $get_default_value('files', NULL),
      '#upload_location' => 'temporary://',
      '#required' => TRUE,
      '#upload_validators' => array(
        'file_validate_extensions' => $extensions,
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );

  if (module_exists('islandora_ocr')) {
    $upload_form['language'] = array(
      '#access' => module_exists('islandora_ocr'),
      '#title' => t('Language'),
      '#type' => 'select',
      '#description' => t('Please select the language the page is written in.'),
      '#options' => $languages,
      '#default_value' => $get_default_value('language', $default_language),
    );
    $upload_form['ocr_ignored_derivatives'] = islandora_ocr_get_ignored_derivatives_fieldset();
  }

  return $upload_form;
}

/**
 * Sets RELS-EXT/RELS-INT properties and creates the 'OBJ' datastream.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_paged_content_upload_page_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  $object = islandora_ingest_form_get_object($form_state);
  $file = file_load($form_state['values']['file']);
  $parent = $form_state['parent'];
  $pages = islandora_paged_content_get_pages($parent);
  $num_pages = count($pages) + 1;
  $rels_ext = $object->relationships;

  $label = $file->filename;
  // Change the label of the page to the sequence number if variable is set.
  if (variable_get('islandora_paged_content_page_label', FALSE)) {
    $label = $num_pages;
  }
  $object->label = $label;

  // Add the OCR creation flags before the datastream is updated.
  if (module_exists('islandora_ocr')) {
    module_load_include('inc', 'islandora_ocr', 'includes/derivatives');
    $language = isset($form_state['values']['language']) ? $form_state['values']['language'] : NULL;
    islandora_ocr_set_generating_rels_ext_statements($object, !$form_state['values']['ignore_ocr'], !$form_state['values']['ignore_hocr'], $language);
  }
  islandora_paged_content_update_datastream($object, drupal_realpath($file->uri), 'OBJ', NULL, NULL, 'M', FALSE);
  // Update RELS-EXT properties, page/sequence/etc, and append the page at the
  // end of the book.
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isPageOf', $parent->id);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', (string) $num_pages, TRUE);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isPageNumber', (string) $num_pages, TRUE);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isSection', '1', TRUE);
  islandora_paged_content_set_relationship($rels_ext, FEDORA_RELS_EXT_URI, 'isMemberOf', $parent->id);
}
