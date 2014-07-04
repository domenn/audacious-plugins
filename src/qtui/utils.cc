/*
 * utils.cc
 * Copyright 2014 Michał Lipski
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <QFileDialog>

#include <libaudcore/drct.h>
#include <libaudcore/index.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

namespace Utils
{
    static void directoryEntered (const QString & path)
    {
        aud_set_str ("audgui", "filesel_path", path.toUtf8 ().constData ());
    }

    void openFilesDialog (bool add = false)
    {
        QFileDialog dialog (0, add ? "Add Files" : "Open Files", QString (aud_get_str ("audgui", "filesel_path")));
        dialog.setFileMode (QFileDialog::ExistingFiles);

        QObject::connect (&dialog, &QFileDialog::directoryEntered, directoryEntered);

        if (add)
            dialog.setLabelText (QFileDialog::Accept, "Add");

        if (dialog.exec ())
        {
            Index<PlaylistAddItem> files;
            Q_FOREACH (QUrl url, dialog.selectedUrls ())
                files.append ({ String(url.toEncoded ().constData ()) });
            if (add)
              aud_drct_pl_add_list (std::move (files), -1);
            else
              aud_drct_pl_open_list (std::move (files));
        }
    }

    void addFilesDialog ()
    {
        openFilesDialog (true);
    }
}
