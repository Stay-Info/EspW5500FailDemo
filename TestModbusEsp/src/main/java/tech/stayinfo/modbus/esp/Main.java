package tech.stayinfo.modbus.esp;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.time.Instant;
import java.util.Arrays;
import java.util.Locale;
import java.util.Objects;

public class Main {

    private static final Logger logger = LoggerFactory.getLogger(Main.class);

    public static final String IP_ARG = "ip";
    public static final String KEEP_ALIVE = "keep";

    public static void main(String[] args) {

       TcpModbus modbus = new TcpModbus(extractArgs(IP_ARG, args));
       String keepAliveStr = extractArgs(KEEP_ALIVE, args);
        modbus.setKeepAlive(keepAliveStr != null
                && (keepAliveStr.equalsIgnoreCase("true") || keepAliveStr.equals("1")));
        long sleep = 1;
        while(true){
            try {
                int[] registers = modbus.readRegisters(0,4);
                logger.info(Arrays.toString(registers));
                sleep = 1;
            } catch (IOException e) {
                sleep = sleep* 2;
                logger.error("{} : Unable to read registers[{}], will retry in {} seconds"
                        , Instant.now(),findCause(e).getMessage(),sleep);
            }
            try {
                Thread.sleep(sleep*1000);
            } catch (InterruptedException ex) {
                throw new RuntimeException(ex);
            }
        }
    }


    /**
     * Finds the value of the specified key. Argument format must be {@code -key=value}
     * @param key They key looked for
     * @param args All arguments
     * @return the value of the key if found, null otherwise
     */
    public static String extractArgs(String key, String[] args){
        int patternLength = key.length() +2;
        for (String arg : args){
            logger.info(arg);
            if (arg != null && arg.startsWith("-"+key+"=")){
                return arg.substring(patternLength);
            }
        }
        return null;
    }

    public static Throwable findCause(Throwable throwable) {
        Objects.requireNonNull(throwable);
        Throwable rootCause = throwable;
        while (rootCause.getCause() != null && rootCause.getCause() != rootCause) {
            rootCause = rootCause.getCause();
        }
        return rootCause;
    }
}