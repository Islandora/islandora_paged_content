<?php

/**
 * @file
 * Callbacks used in derivatives hook.
 */

/**
 * Generate a PDF derivative.
 */
function islandora_paged_content_derivatives_pdf(AbstractObject $object, $force = TRUE) {
  if ($force || !isset($object['PDF'])) {
    module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
    $pdf_file = islandora_paged_content_page_derive_pdf($object);
    if (!$pdf_file) {
      return FALSE;
    }
    $ret = islandora_paged_content_update_datastream($object, $pdf_file, 'PDF');
    file_unmanaged_delete($pdf_file);

    if ($ret) {
      return array(
        'success' => TRUE,
        'messages' => array(
          array(
            'message' => t('Generated %dsid on %pid.'),
            'message_sub' => array(
              '%dsid' => 'PDF',
              '%pid' => $object->id,
            ),
            'type' => 'dsm',
            'severity' => 'status',
          ),
        ),
      );
    }
    else {
      return array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('Failed to generated %dsid on %pid.'),
            'message_sub' => array(
              '%dsid' => 'PDF',
              '%pid' => $object->id,
            ),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
  }
}

/**
 * Add thumbnail to parent, based on first member.
 */
function islandora_paged_content_derive_thumbnail_on_parent(AbstractObject $object, $force = TRUE) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  $book_pid = islandora_paged_content_get_relationship($object->relationships, FEDORA_RELS_EXT_URI, 'isMemberOf', NULL);
  $book_object = islandora_object_load($book_pid);
  if ($book_object && ($force || !isset($book_object['TN']))) {
    if (islandora_paged_content_update_paged_content_thumbnail($book_object)) {
      return array(
        'success' => TRUE,
        'messages' => array(
          array(
            'message' => t('Updated @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $book_object->id,
            ),
            'type' => 'dsm',
            'severity' => 'status',
          ),
        ),
      );
    }
    else {
      return array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('Failed to update @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $book_object->id,
            ),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
  }
}

/**
 * Add thumbnail, based on first member.
 */
function islandora_paged_content_derive_thumbnail_of_parent(AbstractObject $book_object, $force = TRUE) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  if ($force || !isset($book_object['TN'])) {
    if (islandora_paged_content_update_paged_content_thumbnail($book_object)) {
      return array(
        'success' => TRUE,
        'messages' => array(
          array(
            'message' => t('Updated @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $book_object->id,
            ),
            'type' => 'dsm',
            'severity' => 'status',
          ),
        ),
      );
    }
    else {
      return array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('Failed to update @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $book_object->id,
            ),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
  }
}
