/* $Id: GameConfigurationForm.cs 14 2011-12-22 00:41:30Z piranther $
 *
 * Description: Sets up the form for configuring settings for the current game.
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
    public partial class GameConfigurationForm : Form
    {
        public GameConfigurationForm(PropertyGrid aGameGrid)
        {
            InitializeComponent();

            aGameGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            aGameGrid.Location = new System.Drawing.Point(0, 0);
            aGameGrid.Name = "propertyGrid1";
            aGameGrid.Size = new System.Drawing.Size(284, 262);
            aGameGrid.TabIndex = 0;

            this.Controls.Add(aGameGrid);
        }
    }
}