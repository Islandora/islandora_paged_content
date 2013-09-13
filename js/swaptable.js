(function ($) {

/**
 * jQuery Helper function for determining order.
 *
 * Very dumb method doesn't test for the same element or not siblings etc.
 */
$.fn.isAfter = function(sel){
  return this.index() > $(sel).index();
}

/**
 * jQuery Helper function for determining order.
 *
 * Very dumb method doesn't test for the same element or not siblings etc.
 */
$.fn.isBefore= function(sel){
  return !this.isAfter(sel);
}

/**
 * Two tables with drag and drop rows to move rows between them.
 */
Drupal.behaviors.swapTable = {
  attach: function (context, settings) {
    for (var base in settings.swapTable) {
      $('#' + base + '-wrapper', context).once('swaptable', function () {
        // Create the new swapTable instance. Save in the Drupal variable
        // to allow other scripts access to the object.
        Drupal.swapTable[base] = new Drupal.swapTable(this, settings.swapTable[base]);
      });
    }
  }
};

/**
 * Constructor for the swapTable object.
 *
 * @param wrapper
 *   The root element in the template.
 * @param settings
 *   Settings for the swap tables.
 */
Drupal.swapTable = function (wrapper, settings) {
  var self = this;

  // Required object variables.
  this.wrapper = wrapper;
  this.input = $('input', wrapper)[0];
  this.table = $('table.swaptable-wrapper', wrapper)[0];
  this.tables = $('table.swaptable', this.table);
  this.left = this.tables[0];
  this.right = this.tables[1];
  this.settings = settings;

  // Add the actual ID of the row to each row in left tables.
  // ID's are assumed to be unique and also assumed not to contain spaces.
  $('tbody > tr', this.left).each(function(index) {
    $.data(this, 'id', settings.items[index]);
  });

  // Add the actual ID of the row to each row in right tables.
  // ID's are assumed to be unique and also assumed not to contain spaces.
  $('tbody > tr', this.right).each(function(index) {
    $.data(this, 'id', settings.items[index]);
  });

  // Add a link before the table for users to show or hide weight columns.
  $(this.table).before($('<a href="#" class="swaptable-toggle-original-ordering"></a>')
    .attr('title', Drupal.t('Show the original order of items.'))
    .click(function () {
      // @todo Sort out this cookie stuff.
      if ($.cookie('Drupal.swapTable.showOriginal') == 1) {
        self.hideColumns();
      }
      else {
        self.showColumns();
      }
      return false;
    })
    .wrap('<div class="swaptable-toggle-original-ordering-wrapper"></div>')
    .parent()
  );

  // Initialize the specified columns.
  self.initColumns();

  // Create placeholders to model empty spaces when dragging the selected
  // rows above the tables.
  this.createPlaceholder(this.left);
  this.createPlaceholder(this.right);

  // Listen to click events to select rows in the left table.
  $('tbody > tr', this.left).bind("click", function(event) {
    self.selectRow(event, self.left, this);
  });

  // Listen to click events to select rows in the right table.
  $('tbody > tr', this.right).bind("click", function(event) {
    self.selectRow(event, self.right, this);
  });

  // Configure dragging for both tables, ignore placeholders as they can't be
  // dragged.
  $('tbody > tr:not(.ui-placeholder)', this.tables).draggable({
    cursor: "move",
    revert: "invalid",
    refreshPositions: true,
    containment: this.table,
    helper: function (event) {
      var content = '';
      var selected = $(event.currentTarget).siblings('.ui-selected').andSelf();
      var content = $(selected[0]).clone(false).attr('id', null).attr('class', null);
      $('.ordering', content).remove();
      if (selected.length > 1) {
        $('.original-ordering', content).text(Drupal.t('Multiple Selected: #') + selected.length);
        $('.original-ordering', content).attr('css', null).show();
      }
      return $('<div/>').attr('id', 'draggingContainer').append(content);
    },
    start: function (event, ui) {
      var table = $(this).parents('table')[0];
      var otherTable = (table === self.left) ? self.right : self.left;
      // Cancel selections in other table now that dragging has begun.
      $('tr.ui-selected', otherTable).removeClass('ui-selected');
      // Store the selected values.
      var selected = $(this).siblings('.ui-selected').andSelf();
      self.selectElements(this, selected);
      // Move the place holder to the selected row.
      $(this).before($('.ui-placeholder', table).show());
      // Hide the current selection until it's dragged outside of this table.
      selected.hide();
    },
    stop: function (event, ui) {
      // Hide all place holders.
      $('.ui-placeholder', self.table).hide();
      // Show all rows.
      $('tr:hidden:not(.ui-placeholder)', self.tables).show();
      // Deselect everything.
      $('tr.ui-selected', self.tables).removeClass('ui-selected');
      // Forget what was selected.
      self.select(this, null);
      // Correct ordering for both tables.
      self.updateOrdering(self.left);
      self.updateOrdering(self.right);
    },
  });
  // Configure drop locations, ignore placeholder elements as they need to
  // ignore the over events.
  $('tbody > tr:not(.ui-placeholder)', this.tables).droppable({
    tolerance: 'intersect',
    over: function(event, ui) {
      var table = $(this).parents('table')[0];
      var selected = self.selected(ui.draggable[0]);
      self.overTable(table, selected);
      self.movePlaceholder(table, this);
      self.updateOrdering(table, selected);
    },
    drop: function(event, ui) {
      var table = $(this).parents('table')[0];
      var selected = self.selected(ui.draggable[0]);
      self.drop(table, this, selected);
    }
  });
};

/**
 * Marks the given ids as selected for the given draggable element.
 */
Drupal.swapTable.prototype.select = function (draggable, ids) {
  $.data(draggable, 'selected', ids);
}

/**
 * Gets the selected elements.
 */
Drupal.swapTable.prototype.selected = function (draggable) {
  return $.data(draggable, 'selected');
}

/**
 * Meant to be called in the context of the element.
 */
Drupal.swapTable.prototype.getRowID = function () {
  return this.id.substr(this.id.indexOf('-') + 1);
}

/**
 * Selects the given elements and saves them with the given draggable element.
 */
Drupal.swapTable.prototype.selectElements = function (draggable, selected) {
  this.select(draggable, selected.map(this.getRowID));
}

/**
 * Gets all the given elements within the given table.
 */
Drupal.swapTable.prototype.getSelectedElements = function (table, selected) {
  // All rows id's are prefixed with either 'left-' or 'right-' which is the
  // name of the table they belong to.
  var prefix = $(table).attr('name') + '-';
  return $(selected).map(function() {
    return document.getElementById(prefix + this);
  });
}

/**
 * Selection is "stored" by attaching the class 'ui-selected' to rows.
 *
 * @todo Store list and forget conditionally when doing one multi-select
 * immediately after the other, just like on OSX.
 */
Drupal.swapTable.prototype.selectRow = function (event, table, row) {
  // Store the last selected row for building the multi-select list.
  var last = $.data(table, 'last');
  // Shift indicates an intention to select multiple.
  if (event.shiftKey && !event.metaKey && last) {
    var id = '#' + row.id;
    var list = $(row).isAfter($(last)) ? $(last).nextUntil(id) : $(last).prevUntil(id);
    list.add(row).addClass('ui-selected');
  }
  // Select/Deselect only the current selection.
  else if (event.metaKey) {
    $(row).toggleClass('ui-selected');
    $.data(table, 'last', row);
  }
  // Select only the given row.
  else {
    $(row).addClass('ui-selected').siblings().removeClass('ui-selected');
    $.data(table, 'last', row);
  }
}

/**
 * Drop callback for droppable elements.
 */
Drupal.swapTable.prototype.drop = function (table, row, selected) {
  var self = this;
  var otherTable = (table === self.left) ? self.right : self.left;
  // Move the selected elements to placeholder location.
  // Mark as moved.
  $(row).after(this.getSelectedElements(table, selected).addClass('ui-modified'));
  $(row).after(this.getSelectedElements(otherTable, selected).addClass('ui-modified'));
  // Enforce new ordering on the other table.
  var ids = $('tbody > tr:not(.ui-placeholder)', table).map(this.getRowID);
  var rows = this.getSelectedElements(otherTable, ids);
  $('tbody', otherTable).append(rows);
  // Serialize the results to the input.
  var value = rows.map(function() {
    return $.data(this, 'id');
  }).get().join(' ');
  $(this.input).val(value);
}

/**
 * Callback for when a droppable element is hovering over the given table.
 */
Drupal.swapTable.prototype.overTable = function(table, selected) {
  var otherTable = (table === this.left) ? this.right : this.left;
  // Show the currently selected rows only if they belong to the other table.
  this.getSelectedElements(table, selected).hide();
  $('tr:hidden', otherTable).show();
  // Show the place holder on only the current table
  $('tr.ui-placeholder', table).show();
  $('tr.ui-placeholder', otherTable).hide();
}

/**
 * Creates a place holder element for the given table.
 */
Drupal.swapTable.prototype.createPlaceholder = function (table) {
  var self = this;
  $('<tr class="ui-placeholder"><td class="ordering">-1</td></tr>')
    .hide()
    .droppable({
      drop: function(event, ui) {
        var table = $(this).parents('table')[0];
        var selected = self.selected(ui.draggable[0]);
        self.drop(table, this, selected);
      }
    }).prependTo($('tbody', table));
}

/**
 * Move the place holder to before or after the given row.
 */
Drupal.swapTable.prototype.movePlaceholder = function(table, row) {
  // Move placeholder to current drop point.
  var placeholder = $('.ui-placeholder', table);
  var list = $('tbody > tr:visible', table);
  if (list.index(row) > list.index(placeholder)) {
    $(row).after(placeholder);
  }
  else {
    $(row).before(placeholder);
  }
}

/**
 * Update the ordering and classes for the given table.
 */
Drupal.swapTable.prototype.updateOrdering = function(table, selected) {
  selected = selected || [];
  var count = 1;
  $('tbody > tr:visible', table).each(function() {
    // Restripe the table.
    $(this).removeClass('odd even').addClass(count % 2 ? 'odd' : 'even');
    // Reorder the displayed values to always be in order.
    $('td.ordering', this).text(count);
    count += $(this).hasClass('ui-placeholder') ? selected.length : 1;
  });
}

/**
 * Initialize columns containing form elements to be hidden by default.
 *
 * Identify and mark each cell with a CSS class so we can easily toggle
 * show/hide it. Finally, hide columns if user does not have a
 * 'Drupal.swapTable.showOriginal' cookie.
 */
Drupal.swapTable.prototype.initColumns = function () {
  // Now hide cells and reduce colspans unless cookie indicates previous choice.
  // Set a cookie if it is not already present.
  if ($.cookie('Drupal.swapTable.showOriginal') === null) {
    $.cookie('Drupal.swapTable.showOriginal', 0, {
      path: Drupal.settings.basePath,
      // The cookie expires in one year.
      expires: 365
    });
    this.hideColumns();
  }
  // Check cookie value and show/hide weight columns accordingly.
  else {
    if ($.cookie('Drupal.swapTable.showOriginal') == 1) {
      this.showColumns();
    }
    else {
      this.hideColumns();
    }
  }
};

/**
 * Hide the columns containing weight/parent form elements.
 * Undo showColumns().
 */
Drupal.swapTable.prototype.hideColumns = function () {
  // Hide weight/parent cells and headers.
  $('.swaptable-hide', '.swaptable-processed').hide();
  // Change link text.
  $('.swaptable-toggle-original-ordering').text(Drupal.t('Show original order'));
  // Change cookie.
  $.cookie('Drupal.swapTable.showOriginal', 0, {
    path: Drupal.settings.basePath,
    // The cookie expires in one year.
    expires: 365
  });
};

/**
 * Show the columns containing weight/parent form elements
 * Undo hideColumns().
 */
Drupal.swapTable.prototype.showColumns = function () {
  // Show weight/parent cells and headers.
  $('.swaptable-hide', '.swaptable-processed').show();
  // Change link text.
  $('.swaptable-toggle-original-ordering').text(Drupal.t('Hide original order'));
  // Change cookie.
  $.cookie('Drupal.swapTable.showOriginal', 1, {
    path: Drupal.settings.basePath,
    // The cookie expires in one year.
    expires: 365
  });
};

})(jQuery);
