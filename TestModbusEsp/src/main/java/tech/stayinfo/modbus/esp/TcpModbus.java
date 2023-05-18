package tech.stayinfo.modbus.esp;

import com.intelligt.modbus.jlibmodbus.Modbus;
import com.intelligt.modbus.jlibmodbus.exception.ModbusIOException;
import com.intelligt.modbus.jlibmodbus.exception.ModbusNumberException;
import com.intelligt.modbus.jlibmodbus.exception.ModbusProtocolException;
import com.intelligt.modbus.jlibmodbus.master.ModbusMaster;
import com.intelligt.modbus.jlibmodbus.master.ModbusMasterFactory;
import com.intelligt.modbus.jlibmodbus.tcp.TcpParameters;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;


class TcpModbus {

	private static final Logger logger = LoggerFactory.getLogger(TcpModbus.class);


	private String addr = null; //the slave's address
	private int port = Modbus.TCP_PORT;

	private boolean keepAlive = false;

	private int modbusID = 1;

	private final TcpParameters tcpParameters = new TcpParameters();
	private ModbusMaster con;
	
	TcpModbus(String host) {
		setHost(host);
	}

	private void configure() throws ModbusIOException{
		try {
			// Prepare the connection
			this.closeConnection();
			InetAddress address = InetAddress.getByName(this.addr);
			tcpParameters.setHost(address);
			tcpParameters.setKeepAlive(keepAlive);
			tcpParameters.setPort(this.port);
			Modbus.setAutoIncrementTransactionId(true);
			con = ModbusMasterFactory.createModbusMasterTCP(tcpParameters);
			con.setResponseTimeout(900);
		}catch(UnknownHostException e) {
			throw new ModbusIOException(e);
		}

	}

	public void setHost(String host) {
		if (this.addr != null && this.addr.equals(host)) return;
		if (host == null) throw new NullPointerException("Host can't be null");
		this.addr = host;
		this.closeConnection();
	}

	public void setKeepAlive(boolean keepAlive) {
		this.keepAlive = keepAlive;
		this.closeConnection();
	}

	public void setPort(int port) {
		this.port = port;
		this.closeConnection();
	}

	private int[] readRegisters(int modbusId, int firstRegister, int numberRegister) throws IOException {
		return executeRequest(() -> con.readInputRegisters(modbusId, firstRegister, numberRegister));
	}


	public void closeConnection() {
		if (con!=null) {
			if (con.isConnected()) {
				try {
					con.disconnect();
					logger.info("Connection modbus fermée à : "+this.addr);
				}catch(Exception e) {
					logger.warn("Echec de la fermeture de la connection modbus à "+this.addr,e);
				}
			}
			con = null;
		}
	}
	
	private <T> T executeRequest(Requester<T> requester) throws IOException {
		try {
			boolean newConnection = false;
			if (con== null) {
				configure();
				newConnection = true;
			}
			T t = requester.executeRequest();
			if (newConnection) logger.info("Connection modbus established to : "+this.addr);
			return t;
		} catch (ModbusProtocolException | ModbusNumberException | ModbusIOException e) {
			this.closeConnection();
			throw new IOException(e);
		}
	}

	public int getModbusID() {
		return modbusID;
	}

	public void setModbusID(int modbusID) {
		this.modbusID = modbusID;
		this.closeConnection();
	}

	public int[] readRegisters(int firstRegister, int numberRegister) throws IOException {
		return this.readRegisters(modbusID, firstRegister, numberRegister);
	}


	private boolean writeRegister(int slaveId, int register, int value) throws IOException {
		return executeRequest(() -> {
			con.writeSingleRegister(slaveId, register, value);
			return true;
		});
	}

	public boolean writeRegister(int register, int value) throws IOException {
		return this.writeRegister(modbusID, register, value);
	}

	public String getHost() {
		return this.addr;
	}

	public int getPort() {
		return port;
	}

	public boolean isKeepAlive() {
		return keepAlive;
	}

	@FunctionalInterface
	private interface Requester<T>{
		T executeRequest() throws ModbusProtocolException,  ModbusNumberException, ModbusIOException;
	}
}
