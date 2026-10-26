import brut.androlib.smali.SmaliBuilder;
import brut.androlib.smali.SmaliDecoder;
import com.android.tools.smali.dexlib2.dexbacked.DexBackedDexFile;
import com.android.tools.smali.dexlib2.dexbacked.ZipDexContainer;
import com.android.tools.smali.dexlib2.Opcodes;
import com.android.tools.smali.dexlib2.writer.builder.DexBuilder;
import com.android.tools.smali.dexlib2.writer.io.FileDataStore;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Stream;

// Bak.java — smali disassemble/assemble wrapper over apktool 3.0.3
// (bitbucket-hosted standalone smali/baksmali jars are unreachable here, but
//  apktool 3.0.3 embeds the full smali/baksmali 3.0.9-dev libraries).
//
//   disassemble:  java Bak d <in.apk> <outDir> [entryName]
//                 (entryName default "classes.dex"; the APK zip container
//                  is scanned for dex entries)
//   assemble:     java Bak a <smaliDir> <out.dex> <api>
public class Bak {
    @SuppressWarnings("unchecked")
    public static void main(String[] args) throws Exception {
        String mode = args[0];
        if (mode.equals("d")) {
            File apk = new File(args[1]);
            File out = new File(args[2]);
            String want = args.length > 3 ? args[3] : "classes.dex";
            SmaliDecoder dec = new SmaliDecoder(apk, false);
            ZipDexContainer c = dec.mDexContainer;
            TreeMap<String, Object> entries = (TreeMap<String, Object>) (TreeMap) c.getEntries();
            boolean did = false;
            for (Map.Entry<String, Object> e : entries.entrySet()) {
                if (!e.getKey().endsWith(".dex")) continue;
                if (!e.getKey().equals(want)) continue;
                DexBackedDexFile dex = (DexBackedDexFile) e.getValue();
                dec.decodeFile(dex, out);
                System.out.println("disassembled " + e.getKey() + " (api=" + dec.mInferredApiLevel.get()
                        + ") -> " + out);
                did = true;
            }
            if (!did) {
                System.out.println("entries: " + entries.keySet());
                throw new RuntimeException("dex entry not found: " + want);
            }
        } else if (mode.equals("a")) {
            File dir = new File(args[1]);
            File out = new File(args[2]);
            int api = Integer.parseInt(args[3]);
            DexBuilder dex = new DexBuilder(new Opcodes(api));
            SmaliBuilder b = new SmaliBuilder(api);
            int n = 0;
            try (Stream<Path> paths = Files.walk(dir.toPath())) {
                for (Path p : (Iterable<Path>) paths.filter(x -> x.toString().endsWith(".smali"))::iterator) {
                    if (b.buildFile(p.toFile(), dex)) n++;
                }
            }
            dex.writeTo(new FileDataStore(out));
            System.out.println("assembled " + n + " smali files -> " + out + " (" + out.length() + " bytes)");
        } else {
            throw new RuntimeException("mode? d|a");
        }
    }
}
