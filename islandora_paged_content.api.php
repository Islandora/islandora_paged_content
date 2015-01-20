<?php

/**
 * @file
 * This file documents all available hook functions to manipulate data.
 */

/**
 * Identifies content models whose children are paged content.
 */
function hook_islandora_paged_content_content_model_registry() {
  return array(
    'somecmodel',
  );
}
