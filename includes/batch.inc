<?php

/**
 * Sets up a batch operation.
 *
 * @param array $operands
 *   All the pages to operate on.
 * @param array $context
 *   The context of this batch operation.
 * @param array $message
 *   Values are used as parameters to t() and drupal_set_message() that will be
 *   displayed when the batch is finished. Placeholders include, but are not
 *   limited to: @successful, @failed, @total.
 *
 *  @code
 *    array(
 *      ...
 *      array('Processed @successful of @total. Created @pages pages.', array(
 *            '@pages' => $pages), 'status')
 *      ...
 *    )
 *  @endcode
 *
 * @return mixed
 *   The current operand the operation will process.
 */
function islandora_paged_content_start_batch_operation(array $operands, &$context, array $message = array()) {
  if (!isset($context['sandbox']['progress'])) {
    $results =& $context['results'];
    if (empty($results)) {
      // "files" will be deleted upon failure or success of the batch.
      // "successful" an array of boolean values indicating operation success.
      // "messages" array of messages to display on finishing the batch
      // values are used as parameters to t() and drupal_set_message().
      $results = array(
        'files' => array(),
        'successful' => array(),
        'messages' => array(),
      );
    }
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['total'] = count($operands);
    $context['results']['messages'][] = $message;
  }
  $operand_index = $context['sandbox']['progress'];
  return isset($operands[$operand_index]) ? $operands[$operand_index] : FALSE;
}

/**
 * Ends the batch operation.
 *
 * @param array $context
 *   The context of this batch operation.
 * @param int $count
 *   The number of steps taken in the operation, typically 1.
 * @param array $message
 *   Values are used as parameters to t() and drupal_set_message() that will be
 *   displayed when the batch is finished.
 *
 * @see islandora_paged_contnet_start_batch_operation()
 *
 * @return bool
 *   TRUE if the batch operation is complete, FALSE otherwise.
 */
function islandora_paged_content_end_batch_operation(&$context, $count = 1, $message = array()) {
  $context['sandbox']['progress'] += $count;
  if ($context['sandbox']['progress'] != $context['sandbox']['total']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['total'];
    return FALSE;
  }
  $context['results']['messages'][] = $message;
  return TRUE;
}

/**
 * The batch 'finished' callback.
 */
function islandora_paged_content_batch_finished($success, $results, $operations) {
  if ($success) {
    // Delete any lingering files.
    foreach ($results['files'] as $file) {
      if (file_exists($file)) {
        file_unmanaged_delete($file);
      }
    }
    // Build generic place holders for the message.
    $total = count($results['successful']);
    $successful = count(array_filter($results['successful']));
    $failed = $total - $successful;
    $place_holders = array(
      '@successful' => $successful,
      '@failed' => $failed,
      '@total' => $total,
    );
    // Display all messages/errors/warnings.
    $messages = array_filter($results['messages']);
    foreach ($messages as $message) {
      $vars = isset($message[1]) ? array_merge($message[1], $place_holders) : $place_holders;
      $status = isset($message[2]) ? $message[2] : 'status';
      $message = t($message[0], $vars);
      drupal_set_message(check_plain($message), $status);
    }
  }
  else {
    // Generic Error Message.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
                 '%error_operation' => $error_operation[0],
                 '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
}
