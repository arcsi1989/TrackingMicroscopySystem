/*
 Copyright (c) 2013, Andreas Ziegler

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 This software is provided 'as-is', without any express or implied warranty.
*/

#ifndef STORAGEPATH_H
#define STORAGEPATH_H

// included dependencies
#include <QDialog>
#include <QShortcut>
#include "ui_StoragePath.h"

namespace Ui
{
  // forward declared dependencies
  class StoragePath;
}

namespace tracker
{
  // forward declared dependencies
  class MainWindow;

  /**
  * @class StoragePath StoragePath.h
  * @brief Responsible for the StoragePath settings window.
  */
  class StoragePath : public QDialog
  {
    Q_OBJECT

  public:
    /** The constructor
     * @brief Initializes the pointer *mMainWindow, set the connection between the shortcut "F8" and stepUp().
     * @param[in] *parent Pointer to the parent object
     */
    explicit StoragePath(QWidget *parent = 0);

    /** The deconstructor
      *
      */
    ~StoragePath();

    Ui::StoragePath *ui;                                                   /**< Pointer to the StoragePath user interface */
  private slots:
    /**
     * @brief Set the filname in FileNameLineEdit, when the file name is not empty.
     */
    void on_storageLocationButton_clicked();

    /**
     * @brief Set/Update the spin box value from the MainWindow.
     * @param value The spin box value from MainWindow
     */
    void setSpinBoxValue(int value);

    /**
     * @brief Set/Update the file name in FileNameLineEdit.
     * @param arg1 File name as QString
     */
    void setFileName(const QString *arg1);

    /**
     * @brief Updats the value from the SpinBox in MainWindow.
     * @param arg1 The current value
     */
    void on_storageRunSpinBox_valueChanged(int arg1);

    /**
     * @brief Updats the file name from the FileNameLineEdit in MainWindow.
     * @param arg1 File name as QString
     */
    void on_storageFilenameEdit_textChanged(const QString &arg1);

  private:
    MainWindow *mMainWindow;                        /**< Pointer to the MainWindow object */

    QShortcut*           mNextRunShortcut;          ///< Extra shortcut for 'next run'
  };
}

#endif // STORAGEPATH_H
