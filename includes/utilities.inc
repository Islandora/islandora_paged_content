<?php

/**
 * @file
 * This file contains all the utility functions for paged content.
 * There are functions for:
 *   Getting pages for a paged content object
 *   Generating pdfs
 *   Generating image derivatives
 *   Generating OCR and HOCR
 */

/**
 * Gets all the pages in the given paged content object.
 *
 * @param AbstractObject $object
 *   The paged content object to fetch the pages from.
 *
 * @return array
 *   All the pages in the given paged content object.
 */
function islandora_paged_content_get_pages(AbstractObject $object) {
  $query = <<<EOQ
PREFIX islandora-rels-ext: <http://islandora.ca/ontology/relsext#>
SELECT ?pid ?page ?label ?width ?height
FROM <#ri>
WHERE {
  ?pid <fedora-rels-ext:isMemberOf> <info:fedora/{$object->id}> ;
       <fedora-model:label> ?label ;
       islandora-rels-ext:isSequenceNumber ?page .
  OPTIONAL {
    ?pid <fedora-view:disseminates> ?dss .
    ?dss <fedora-view:disseminationType> <info:fedora/*/JP2> ;
         islandora-rels-ext:width ?width ;
         islandora-rels-ext:height ?height .
 }
}
ORDER BY ?page
EOQ;

  $results = $object->repository->ri->sparqlQuery($query);

  // Get rid of the "extra" info...
  $map = function($o) {
    foreach ($o as $key => &$info) {
      $info = $info['value'];
    }

    $o = array_filter($o);

    return $o;
  };
  $pages = array_map($map, $results);

  // Grab the PIDs...
  $get_pid = function($o) {
    return $o['pid'];
  };
  $pids = array_map($get_pid, $pages);

  // If we have some pages, combine our remapped results to produce an array
  // mapping pids to the values for that pid.
  $pages = count($pids) ? array_combine($pids, $pages) : array();

  // Sort the pages into their proper order.
  $sort = function($a, $b) {
    $a = (is_array($a) && isset($a['page'])) ? $a['page'] : 0;
    $b = (is_array($b) && isset($b['page'])) ? $b['page'] : 0;
    if ($a == $b) {
      return 0;
    }
    return ($a < $b) ? -1 : 1;
  };
  uasort($pages, $sort);

  return $pages;
}

/**
 * Function get_page_progression.
 *
 * Gets the page progression to be used in a viewer from the given paged
 * content object.
 *
 * @param AbstractObject $object
 *   The paged content object to fetch the page progression from.
 *
 * @return string
 *   The page progression of the paged content object:  either left to right
 *   'lr' or right to left 'rl'.
 */
function islandora_paged_content_get_page_progression(AbstractObject $object) {
  $query = 'select $page_progression from <#ri> where <info:fedora/';
  $query .= $object->id;
  $query .= '><fedora-rels-ext:hasPageProgression> $page_progression';

  $results = $object->repository->ri->query($query);
  if (count($results) > 0) {
    return $results[0]['page_progression']['value'];
  }
  return 'rl';
}

/**
 * Create/Update a datastream with the given parameters.
 *
 * @param AbstractObject $object
 *   The object to update.
 * @param string $file
 *   The absolute path to the file to use as content for the datastream.
 * @param string $dsid
 *   The datastream ID.
 * @param string $label
 *   The datastream label.
 * @param string $mimetype
 *   The datastream mimetype, if none given it will be guessed from the file
 *   extension.
 * @param string $control_group
 *   The datastream control group.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_paged_content_update_datastream(AbstractObject $object, $file, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
  if (!isset($mimetype)) {
    $mime_detector = new MimeDetect();
    $mimetype = $mime_detector->getMimetype($file);
  }
  $label = isset($label) ? $label : $dsid;

  if (!isset($object[$dsid])) {
    $ds = $object->constructDatastream($dsid, $control_group);
    $ds->mimetype = $mimetype;
    $ds->label = $label;
    $ds->setContentFromFile($file, $copy);
    $object->ingestDatastream($ds);
  }
  else {
    $ds = $object[$dsid];
    $ds->setContentFromFile($file, $copy);
    if ($ds->label != $label) {
      $ds->label = $label;
    }
    if ($ds->mimetype != $mimetype) {
      $ds->mimetype = $mimetype;
    }
  }
  return TRUE;
}

/**
 * Gets the given relationship if one is defined.
 *
 * Assumes only one relationship matches the given parameters.
 * Will return the given default if no relationship is found.
 *
 * @param FedoraRelationships $relationships
 *   The Fedora relationships to check, either RELS-EXT or RELS-INT.
 * @param string $predicate_uri
 *   The predicate URI.
 * @param string $predicate
 *   The predicate.
 * @param mixed $default
 *   The default value to return if no relationship was found.
 *
 * @return mixed
 *   The first matching relationships value if found, otherwise the given
 *   default it returned.
 */
function islandora_paged_content_get_relationship(FedoraRelationships $relationships, $predicate_uri, $predicate, $default = NULL) {
  $results = $relationships->get($predicate_uri, $predicate);
  return count($results) > 0 ? $results[0]['object']['value'] : $default;
}

/**
 * Sets a relationship removing any previous relationships.
 *
 * @param FedoraRelationships $relationships
 *   The Fedora relationships to be modified, either RELS-EXT or RELS-INT.
 * @param string $predicate_uri
 *   The predicate URI.
 * @param string $predicate
 *   The predicate.
 * @param string $object
 *   The object.
 * @param bool $literal
 *   TRUE if the object is a literal, FALSE if it is an object.
 */
function islandora_paged_content_set_relationship(FedoraRelationships $relationships, $predicate_uri, $predicate, $object, $literal = FALSE) {
  $relationships->remove($predicate_uri, $predicate, NULL, $literal);
  $relationships->add($predicate_uri, $predicate, $object, $literal);
}

/**
 * Gets only the enabled ingest derivatives.
 *
 * As defined by the user in the admin form.
 *
 * @param string $namespace
 *   The solution pack namespace whose admin form we're checking
 *
 * @return array
 *   The enabled ingest derivatives.
 */
function islandora_paged_content_get_enabled_ingest_derivatives($namespace) {
  $default_derivatives = drupal_map_assoc(array('pdf', 'image', 'ocr'));
  return variable_get("${namespace}_ingest_derivatives", $default_derivatives);
}

/**
 * Gets the datastream ID of the intended source for the given derivative.
 *
 * @param string $dsid
 *   The derived datastream ID.
 *
 * @return string
 *   The datastream ID of the source for the given derivative.
 */
function islandora_paged_content_get_page_derivative_source_id($dsid) {
  // @todo Change TIFF to OBJ, when done testing.
  $source_datastreams = array(
    'PDF' => 'OBJ',
    'OCR' => 'OBJ',
    'HOCR' => 'OBJ',
    'TN' => 'OBJ',
    'JPG' => 'OBJ',
    'JP2' => 'OBJ',
  );
  return $source_datastreams[$dsid];
}

/**
 * Checks if the given derivative datastream's intended source exists.
 *
 * @param AbstractObject $object
 *   The object to check.
 * @param string $dsid
 *   The derived datastream ID.
 *
 * @return bool
 *   TRUE if the source datastream exists, FALSE otherwise.
 */
function islandora_paged_content_page_has_derivative_source(AbstractObject $object, $dsid) {
  $source_dsid = islandora_paged_content_get_page_derivative_source_id($dsid);
  return isset($object[$source_dsid]);
}

/**
 * Gets the source file for the given, derivative datastream.
 *
 * @param AbstractObject $object
 *   The owner of the datastreams.
 * @param string $dsid
 *   The derived datastream ID.
 *
 * @return string
 *   The path to the source file.
 */
function islandora_paged_content_get_page_derivative_source(AbstractObject $object, $dsid) {
  if (!islandora_paged_content_can_derive($object, $dsid)) {
    return FALSE;
  }
  $source_dsid = islandora_paged_content_get_page_derivative_source_id($dsid);
  $mime_detector = new MimeDetect();
  $ext = $mime_detector->getExtension($object[$source_dsid]->mimeType);

  $base_name = "{$object->id}_{$source_dsid}.{$ext}";

  // Colons mess with some programs...
  $base_name = str_replace(':', '-', $base_name);

  $source_file = file_create_filename($base_name, 'temporary://');
  $object[$source_dsid]->getContent($source_file);
  return drupal_realpath($source_file);
}

/**
 * Checks if the given object can derive the given datastream.
 *
 * @param AbstractObject $object
 *   The object to check.
 * @param string $dsid
 *   The derived datastream ID.
 *
 * @return bool
 *   TRUE if the datastream can be derived, FALSE otherwise.
 */
function islandora_paged_content_can_derive(AbstractObject $object, $dsid) {
  $can_derive = FALSE;
  $to_derive = array(
    'islandora:newspaperCModel',
    'islandora:bookCModel',
    'islandora:pageCModel',
    'islandora:bookPageCModel',
    'islandora:newspaperPageCModel',
  );
  if (array_intersect($to_derive, $object->models)) {
    $check_function = array(
      'PDF' => 'islandora_paged_content_can_create_pdf',
      'OCR' => 'islandora_ocr_can_derive_ocr',
      'HOCR' => 'islandora_ocr_can_derive_ocr',
      'TN' => 'islandora_paged_content_can_create_images',
      'JP2' => 'islandora_paged_content_can_create_images',
      'JPEG' => 'islandora_paged_content_can_create_images',
    );

    if (isset($check_function[$dsid])) {
      if ($dsid == "OCR" || $dsid == 'HOCR') {
        module_load_include('inc', 'islandora_ocr', 'includes/utilities');
      }

      $can_derive = function_exists($check_function[$dsid]) && $check_function[$dsid]();
    }

    $can_derive = $can_derive && islandora_paged_content_page_has_derivative_source($object, $dsid);
  }
  return $can_derive;
}

/**
 * Checks if it is possible to create image files.
 *
 * @return bool
 *   TRUE if it is possible, FALSE otherwise.
 */
function islandora_paged_content_can_create_images() {
  return module_exists('islandora_large_image');
}

/**
 * Checks if it is possible to create PDF files with imagemagick.
 *
 * @return bool
 *   TRUE if it is possible, FALSE otherwise.
 */
function islandora_paged_content_can_create_pdf() {
  // @todo Should also check if imagemagick is configured correctly.
  return module_exists('imagemagick');
}

/**
 * Checks if it is possible to combined PDF files with GhostScript.
 *
 * @return bool
 *   TRUE if it is possible, FALSE otherwise.
 */
function islandora_paged_content_can_combine_pdf() {
  $gs = variable_get('islandora_book_gs', '/usr/bin/gs');
  return is_executable($gs);
}

/**
 * Appends a number of PDF files onto the given PDF file.
 *
 * @param array $file
 *   The PDF files to append onto.
 * @param array $files
 *   The PDF files to append.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_paged_content_pdf_append($file, array $files) {
  $temp_file = "$file.temp.pdf";
  copy($file, $temp_file);
  array_unshift($files, $temp_file);
  $ret = islandora_paged_content_pdf_combine($files, $file);
  file_unmanaged_delete($temp_file);
  return $ret;
}

/**
 * Combines the given PDF files into one output file.
 *
 * @param array $files
 *   The PDF files to be combined in order.
 * @param string $out
 *   The absolute path to the combined PDF file.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function islandora_paged_content_pdf_combine(array $files, $out) {
  $gs = variable_get('islandora_book_gs', '/usr/bin/gs');
  $files = implode(' ', $files);
  $command = "{$gs} -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile={$out} {$files}";
  $output = array(); $ret = 0;
  exec($command, $output, $ret);
  if ($ret != 0) {
    $variables = array(
      '@ret' => $ret,
      '@command' => $command,
      '!output' => implode('<br/>', $output),
    );
    watchdog('islandora_paged_content', 'GhostScript failed to combine PDFs.<br/>Error: @ret<br/>Command: @command <br/>Output !output', $variables, WATCHDOG_ERROR);
    return FALSE;
  }
  return TRUE;
}

/**
 * Creates a PDF derivative for the given Page object.
 *
 * @param AbstractObject $object
 *   The page object that the derivative will be generated from, and added to.
 * @param array $options
 *   The options to be passed to convert.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_paged_content_page_derive_pdf_datastream(AbstractObject $object, array $options = array('-compress' => 'LZW')) {
  $pdf_file = islandora_paged_content_page_derive_pdf($object, $options);
  if (!$pdf_file) {
    return FALSE;
  }
  $ret = islandora_paged_content_update_datastream($object, $pdf_file, 'PDF');
  file_unmanaged_delete($pdf_file);
  return $ret;
}

/**
 * Creates a PDF derivative from the given Page object and convert options.
 *
 * @param AbstractObject $object
 *   The page object that the derivative will be generated from.
 * @param array $options
 *   The options to be passed to convert.
 *
 * @return string
 *   The absolute path to the derived file if successful, FALSE otherwise.
 */
function islandora_paged_content_page_derive_pdf(AbstractObject $object, array $options = array('-compress' => 'LZW')) {
  if (!islandora_paged_content_can_derive($object, 'PDF')) {
    return FALSE;
  }
  $source_file
    = islandora_paged_content_get_page_derivative_source($object, 'PDF');
  $pdf_file = islandora_paged_content_convert_to_pdf($source_file, $options);
  file_unmanaged_delete($source_file);
  return $pdf_file;
}

/**
 * Converts the given image to a PDF file, using the given options.
 *
 * @param string $image_file
 *   The absolute path to the image file.
 * @param array $options
 *   The options to be passed to convert.
 *
 * @return string
 *   The absolute path to the generated PDF if successful, FALSE otherwise.
 */
function islandora_paged_content_convert_to_pdf($image_file, array $options) {
  // Convert options into a string.
  $to_string = function(&$o, $k) {
    $o = "$k $o";
  };
  array_walk($options, $to_string);
  $options = implode(' ', $options);
  $pdf_file = "$image_file.pdf";
  $command_args = "{$options} {$image_file} {$pdf_file}";
  $output = array(); $ret = 0;
  if (_imagemagick_convert_exec($command_args, $output, $ret) !== TRUE) {
    $message = 'ImageMagick failed to create a PDF.<br/>Error: @ret<br/>Command: @command<br/>Output: !output';
    $variables = array(
      '@ret' => $ret,
      '@command' => "convert $command_args",
      '!output' => implode('<br/>', $output),
    );
    watchdog('islandora_paged_content', 'ImageMagick failed to create a PDF.<br/>Error: @ret<br/>Command: @command<br/>Output: !output', $variables, WATCHDOG_ERROR);
    return FALSE;
  }
  return $pdf_file;
}

/**
 * Creates and adds all the image derivatives for the given Page object.
 *
 * @param AbstractObject $object
 *   The page object that the derivative will be generated for.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_paged_content_page_derive_image_datastreams(AbstractObject $object) {
  module_load_include('inc', 'islandora_large_image', 'includes/derivatives');
  $ret = islandora_large_image_create_all_derivatives($object);
  if ($ret) {
    // Add dimensions for the JP2.
    islandora_paged_content_add_dimensions_relationships($object);
  }
  return $ret;
}

/**
 * Add the dimensions to the RELS-INT for the JP2.
 *
 * This makes the dimensions accessible when we query for the pages.
 *
 * @param AbstractObject $object
 *   Page Object
 */
function islandora_paged_content_add_dimensions_relationships(AbstractObject $object, $force = FALSE) {
  $rels = $object['JP2']->relationships;
  $width_rels = $rels->get(ISLANDORA_RELS_EXT_URI, 'width');
  if ($force || empty($width_rels)) {
    // XXX: ... seems a little odd to grab them from the OBJ/tiff, but they're
    // more widely supported...
    $temp_file = drupal_tempnam('temporary://', 'dimensions');
    $object['OBJ']->getContent($temp_file);
    list($width, $height) = getimagesize($temp_file);

    islandora_paged_content_set_relationship($rels, ISLANDORA_RELS_EXT_URI,
        'width', $width, 1
      );
    islandora_paged_content_set_relationship($rels, ISLANDORA_RELS_EXT_URI,
        'height', $height, 1
      );

    drupal_unlink($temp_file);
  }
}

/**
 * Checks if it is possible to update a paged content's thumbnail image.
 */
function islandora_paged_content_can_update_paged_content_thumbnail(AbstractObject $object) {
  $pages = islandora_paged_content_get_pages($object);
  if (count($pages)) {
    $page = array_shift($pages);
    $page = islandora_object_load($page['pid']);
    return isset($page['TN']);
  }
  return FALSE;
}

/**
 * Updates the thumbnail from the first page in the paged content.
 *
 * @param AbstractObject $object
 *   The paged content object to update.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function islandora_paged_content_update_paged_content_thumbnail(AbstractObject $object) {
  if (!islandora_paged_content_can_update_paged_content_thumbnail($object)) {
    return FALSE;
  }
  $pages = islandora_paged_content_get_pages($object);
  $page = array_shift($pages);
  $page = islandora_object_load($page['pid']);
  $mime_detector = new MimeDetect();
  $ext = $mime_detector->getExtension($page['TN']->mimeType);
  $file = drupal_realpath('temporary://{$page->id}_TN.{$ext}');
  $page['TN']->getContent($file);
  $ret = islandora_paged_content_update_datastream($object,
    $file,
    'TN',
    'Thumbnail');
  file_unmanaged_delete($file);
  return $ret;
}

/**
 * Creates and adds the OCR, HOCR datastreams.
 *
 * @param AbstractObject $object
 *   The page object that the derivatives will be generated for.
 * @param array $options
 *   The options for tesseract/gimp.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function islandora_paged_content_page_derive_ocr_datastreams(AbstractObject $object, array $options = NULL) {
  module_load_include('inc', 'islandora_ocr', 'includes/utilities');
  $rels_ext = $object->relationships;
  $options = isset($options) ? $options : array(
    'language' => islandora_paged_content_get_relationship($rels_ext,
      ISLANDORA_RELS_EXT_URI,
      'hasLanguage',
      'eng'),
  );

  module_load_include('inc', 'islandora_ocr', 'includes/derivatives');
  $ret = islandora_ocr_derive_datastreams($object['OBJ'], $options);

  // Check the depth of the image, to see if it can be processed with Tesseract.
  // If it can not, reduce the depth and grayscale it...
  $language = $options['language'];
  islandora_paged_content_set_relationship($rels_ext,
    ISLANDORA_RELS_EXT_URI,
    'hasLanguage',
    $language,
    TRUE);
  return $ret;
}

/**
 * Fetches the given page's dimensions.
 *
 * Firsts attempts to get it from the FITS metadata datastream, if that fails
 * it will use Djatoka.
 *
 * @param AbstractObject $object
 *   The page object.
 *
 * @return array
 *   An associative array containing the following fields.
 *   - width: The width of the image in pixels.
 *   - height: The width of the image in pixels.
 *   If this function should fail for some reason all the fields will be 0.
 */
function islandora_paged_content_get_page_dimensions(AbstractObject $object) {
  $dimensions = array('width' => 0, 'height' => 0);
  $metadata = islandora_paged_content_get_technical_metadata($object);
  $metadata = $metadata ? $metadata : islandora_paged_content_get_page_metadata_from_djatoka($object->id);
  return $metadata ? array_intersect_key($metadata, $dimensions) : $dimensions;
}

/**
 * Fetches the given page's technical metadata.
 *
 * The technical metadata is pull from the techincal metadata datastream created
 * by islandora_fits.
 *
 * @param AbstractObject $object
 *   The page object.
 *
 * @return array
 *   An associative array containing the following fields.
 *   - width: The width of the image in pixels.
 *   - height: The width of the image in pixels.
 *   If this function fails FALSE is returned.
 */
function islandora_paged_content_get_technical_metadata(AbstractObject $object) {
  $dsid = variable_get('islandora_fits_techmd_dsid', 'TECHMD');
  if (empty($object[$dsid])) {
    return FALSE;
  }
  $metadata = FALSE;
  $file = file_create_filename("{object->id}_{$dsid}.xml", 'temporary://');
  $file = drupal_realpath($file);
  if (strpos($object[$dsid]->mimetype, 'xml') !== FALSE && $object[$dsid]->getContent($file)) {
    $doc = simplexml_load_file($file);
    $doc->registerXPathNamespace('ns', 'http://hul.harvard.edu/ois/xml/ns/fits/fits_output');
    $width = (int) array_pop($doc->xpath('/ns:fits/ns:metadata/ns:image/ns:imageWidth'));
    $height = (int) array_pop($doc->xpath('/ns:fits/ns:metadata/ns:image/ns:imageHeight'));
    $metadata = array('width' => $width, 'height' => $height);
  }
  file_unmanaged_delete($file);
  return $metadata;
}

/**
 * Fetches the given page's metadata via Djatoka.
 *
 * @param string $object_id
 *   The PID of the page to fetch the metadata from.
 *
 * @return array
 *   An associative array contatining the following string fields:
 *   - identifier: The URL to the resource.
 *   - imagefile: The path to the temp file being served.
 *   - width: The width of the image in pixels.
 *   - height: The width of the image in pixels.
 *   - dwtLevels: ???
 *   - levels: ???
 *   - compositingLayerCount: ???
 *   If the request to Djatoka fails then FALSE is returned.
 */
function islandora_paged_content_get_page_metadata_from_djatoka($object_id) {
  module_load_include('inc', 'islandora', 'includes/authtokens');
  $datastream_url = url("islandora/object/{$object_id}/datastream/JP2/view", array(
    'absolute' => TRUE,
    'query' => array(
      'token' => islandora_get_object_token($object_id, 'JP2'),
    ),
  ));

  $djatoka_url = variable_get('islandora_paged_content_djatoka_url', 'http://localhost:8080/adore-djatoka');
  $djatoka_url .= (substr($djatoka_url, -1) == '/') ? '' : '/';
  $djatoka_url = url($djatoka_url . "resolver", array(
    'query' => array(
      'url_ver' => 'Z39.88-2004',
      'rft_id' => $datastream_url,
      'svc_id' => 'info:lanl-repo/svc/getMetadata',
    ),
    'external' => TRUE,
  ));
  $request = drupal_http_request($djatoka_url);
  if ($request->code == '200') {
    return drupal_json_decode($request->data);
  }
  return FALSE;
}

/**
 * Returns page derivatives.
 *
 * @param AbstractObject $object
 *   Object requiring derivatives
 * @param String  $context
 *   Indicator of which solution pack calling this function
 *
 * @return array
 *   Structured array of page derivatives
 */
function islandora_paged_content_page_derivatives(AbstractObject $object, $context) {
  $large_image_module_path = drupal_get_path('module', 'islandora_large_image');
  $paged_content_module_path = drupal_get_path('module', 'islandora_paged_content');
  $ocr_module_path = drupal_get_path('module', 'islandora_ocr');
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  $derive = islandora_paged_content_get_enabled_ingest_derivatives($context);
  $derivatives = array();

  if ($derive['pdf']) {
    $derivatives[] = array(
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'PDF',
      'function' => array('islandora_paged_content_derivatives_pdf'),
      'file' => "$paged_content_module_path/includes/derivatives.inc",
    );
  }

  if ($derive['ocr']) {
    $derivatives[] = array(
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'OCR',
      'function' => array('islandora_ocr_derive_ocr'),
      'file' => "$ocr_module_path/includes/derivatives.inc",
    );
    $derivatives[] = array(
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'HOCR',
      'function' => array('islandora_ocr_derive_hocr'),
      'file' => "$ocr_module_path/includes/derivatives.inc",
    );
  }
  if ($derive['image']) {
    $derivatives[] = array(
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'JP2',
      'function' => array('islandora_large_image_create_jp2_derivative'),
      'file' => "$large_image_module_path/includes/derivatives.inc",
    );
    $derivatives[] = array(
      'source_dsid' => 'JP2',
      'destination_dsid' => NULL,
      'function' => array('islandora_paged_content_add_dimensions_relationships'),
      'file' => "$paged_content_module_path/includes/utilities.inc",
    );
    $derivatives[] = array(
      'source_dsid' => 'TN',
      'destination_dsid' => NULL,
      'function' => array('islandora_paged_content_derive_thumbnail_on_parent'),
      'file' => "$paged_content_module_path/includes/derivatives.inc",
    );
    $derivatives[] = array(
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'TN',
      'function' => array('islandora_large_image_create_tn_derivative'),
      'file' => "$large_image_module_path/includes/derivatives.inc",
    );
    $derivatives[] = array(
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'JPG',
      'function' => array('islandora_large_image_create_jpg_derivative'),
      'file' => "$large_image_module_path/includes/derivatives.inc",
    );
  }

  return $derivatives;
}
