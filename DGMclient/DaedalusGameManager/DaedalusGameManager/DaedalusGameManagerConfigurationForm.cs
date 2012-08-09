/* $Id: DaedalusGameManagerConfigurationForm.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: The game manager's configuration form displays a property grid
 * that controls the game manager's configuration.
 *
 * Copyright (c) 2010-2011, Team Daedalus (Mathew Bergt, Jason Buck, Ken Kelley, and
 * Justin Weaver).
 *
 * Distributed under the BSD-new license. For details see the BSD_LICENSE file
 * that should have been included with this distribution. If the source you
 * acquired this distribution from incorrectly removed this file, the license
 * may be viewed at http://www.opensource.org/licenses/bsd-license.php.
 */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace DaedalusGameManager
{
    public partial class DaedalusGameManagerConfigurationForm : Form
    {
        public DaedalusGameManagerConfigurationForm()
        {
            InitializeComponent();

            PropertyGrid propertyGrid = new PropertyGrid();
            propertyGrid.SelectedObject = new DaedalusGameManagerProperties();
            propertyGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            propertyGrid.Location = new System.Drawing.Point(0, 0);
            propertyGrid.Name = "propertyGrid1";
            propertyGrid.Size = new System.Drawing.Size(284, 262);
            propertyGrid.TabIndex = 0;

            this.Controls.Add(propertyGrid);
        }
    }
}