/*
 * Created on Dec 8, 2011
 */
package com.bbn.rpki.test.tasks;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import com.bbn.rpki.test.objects.Util;

/**
 * Task to upload one file
 *
 * @author tomlinso
 */
public class UploadFile implements Task {

  private final File file;
  private final String repository;

  /**
   * @param file
   * @param repository
   */
  public UploadFile(File file, String repository) {
    this.file = file;
    this.repository = repository;
  }

  /**
   * @see com.bbn.rpki.test.tasks.Task#run()
   */
  @Override
  public void run(int epochIndex) {
    List<String> cmd = new ArrayList<String>();
    cmd.add("scp");
    cmd.add(file.getPath());
    cmd.add(repository);
    String[] cmdArray = cmd.toArray(new String[cmd.size()]);
    Util.exec(cmdArray, "UploadFile", false, Util.RPKI_ROOT);
  }

  /**
   * @see com.bbn.rpki.test.tasks.Task#getBreakdownCount()
   */
  @Override
  public int getBreakdownCount(int epochIndex) {
    return 0;
  }

  /**
   * @see com.bbn.rpki.test.tasks.Task#getTaskBreakdown(int)
   */
  @Override
  public TaskBreakdown getTaskBreakdown(int epochIndex, int n) {
    assert false;
    return null;
  }

}