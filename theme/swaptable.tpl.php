<?php
   /**
   * @file
   */
?>
<div id='<?php print $element['#id'] . '-wrapper'; ?>'>
  <table class="swaptable-wrapper">
    <tbody>
      <tr>
        <td valign="top" style="vertical-align: top;"><?php print $left_table; ?></td>
        <td valign="top" style="vertical-align: top;"><?php print $right_table; ?></td>
      </tr>
    </tbody>
  </table>
  <input type="hidden" id="<?php print $element['#id']; ?>" name="<?php print $element['#name']; ?>" value="<?php print $element['#value']; ?>">
</div>
