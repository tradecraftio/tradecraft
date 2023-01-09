package in.freicocore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class FreicoinQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File freicoinDir = new File(getFilesDir().getAbsolutePath() + "/.freicoin");
        if (!freicoinDir.exists()) {
            freicoinDir.mkdir();
        }

        super.onCreate(savedInstanceState);
    }
}
