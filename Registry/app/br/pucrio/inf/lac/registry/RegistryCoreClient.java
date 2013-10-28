package br.pucrio.inf.lac.registry;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.json.JSONException;
import org.json.JSONObject;

import lac.cnclib.net.NodeConnection;
import lac.cnclib.net.NodeConnectionListener;
import lac.cnclib.net.mrudp.MrUdpNodeConnection;
import lac.cnclib.sddl.message.ApplicationMessage;
import lac.cnclib.sddl.message.Message;
import lac.cnclib.sddl.serialization.Serialization;
import modellibrary.RequestInfo;
import modellibrary.ResponseInfo;

public class RegistryCoreClient implements NodeConnectionListener {
	private static String gatewayIP = "127.0.0.1";
	private static int gatewayPort = 5500;
	private MrUdpNodeConnection connection;

	public static void main(String[] args) {
		Logger.getLogger("").setLevel(Level.OFF);

		new RegistryCoreClient();
	}

	public RegistryCoreClient() {
		InetSocketAddress address = new InetSocketAddress(gatewayIP,
				gatewayPort);
		try {
			connection = new MrUdpNodeConnection();
			connection.connect(address);
			connection.addNodeConnectionListener(this);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void connected(NodeConnection remoteCon) {
		System.out.println("[RegistryCoreClient] Conectou, enviando saudação.");

		// REQUEST send
		ApplicationMessage appMessage1 = new ApplicationMessage();
		RequestInfo requestMessage1 = new RequestInfo(
				appMessage1.getSenderID(), "lstNodes", "");
		appMessage1.setContentObject(requestMessage1);
		try {
			remoteCon.sendMessage(appMessage1);
		} catch (IOException e) {
			e.printStackTrace();
		}

		ApplicationMessage appMessage2 = new ApplicationMessage();
		RequestInfo requestMessage2 = new RequestInfo(
				appMessage2.getSenderID(), "srchNodes", "teste");
		appMessage2.setContentObject(requestMessage2);
		try {
			remoteCon.sendMessage(appMessage2);
		} catch (IOException e) {
			e.printStackTrace();
		}

		ApplicationMessage appMessage3 = new ApplicationMessage();
		RequestInfo requestMessage3 = new RequestInfo(
				appMessage3.getSenderID(), "getNode", "teste");
		appMessage3.setContentObject(requestMessage3);
		try {
			remoteCon.sendMessage(appMessage3);
		} catch (IOException e) {
			e.printStackTrace();
		}

		JSONObject xresult = new JSONObject();
		JSONObject xinfo = new JSONObject();

		try {
			xresult.put("uuid", "987654321");
			xresult.put("name", "teste");
			xinfo.put("email", "a@a.com");
			xinfo.put("city", "cidade do usuario");
			xinfo.put("phone", "555-555-555");
			xinfo.put("birthday", "01/01/1970");
			xinfo.put("pass", "senha_fraca");
			xresult.put("info", xinfo.toString());
		} catch (JSONException e1) {
			e1.printStackTrace();
		}

		ApplicationMessage appMessage4 = new ApplicationMessage();
		RequestInfo requestMessage4 = new RequestInfo(
				appMessage4.getSenderID(), "addNode", xresult.toString());
		appMessage4.setContentObject(requestMessage4);
		try {
			remoteCon.sendMessage(appMessage4);
		} catch (IOException e) {
			e.printStackTrace();
		}

		ApplicationMessage appMessage5 = new ApplicationMessage();
		RequestInfo requestMessage5 = new RequestInfo(
				appMessage5.getSenderID(), "delNode", "teste");
		appMessage5.setContentObject(requestMessage5);
		try {
			remoteCon.sendMessage(appMessage5);
		} catch (IOException e) {
			e.printStackTrace();
		}

		ApplicationMessage appMessage6 = new ApplicationMessage();
		RequestInfo requestMessage6 = new RequestInfo(
				appMessage6.getSenderID(), "getNode", "teste");
		appMessage6.setContentObject(requestMessage6);
		try {
			remoteCon.sendMessage(appMessage6);
		} catch (IOException e) {
			e.printStackTrace();
		}

		JSONObject xresult2 = new JSONObject();
		JSONObject xinfo2 = new JSONObject();

		try {
			xresult2.put("uuid", "98765432100000-00000");
			xresult2.put("name", "teste_fica");
			xinfo2.put("email", "a@a.org");
			xinfo2.put("city", "cidade do usuario");
			xinfo2.put("phone", "555-555-555");
			xinfo2.put("birthday", "01/01/1970");
			xinfo2.put("pass", "senha_forte");
			xresult2.put("info", xinfo2.toString());
		} catch (JSONException e1) {
			e1.printStackTrace();
		}

		ApplicationMessage appMessage7 = new ApplicationMessage();
		RequestInfo requestMessage7 = new RequestInfo(
				appMessage7.getSenderID(), "addNode", xresult2.toString());
		appMessage7.setContentObject(requestMessage7);
		try {
			remoteCon.sendMessage(appMessage7);
		} catch (IOException e) {
			e.printStackTrace();
		}

		ApplicationMessage appMessage8 = new ApplicationMessage();
		RequestInfo requestMessage8 = new RequestInfo(
				appMessage8.getSenderID(), "lstNodes", "");
		appMessage8.setContentObject(requestMessage8);
		try {
			remoteCon.sendMessage(appMessage8);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void newMessageReceived(NodeConnection remoteCon, Message message) {
		String className = message.getContentObject().getClass()
				.getCanonicalName();

		System.out
				.println("[RegistryCoreClient] Mensagem recebida do servidor: "
						+ Serialization.fromJavaByteStream(message.getContent())
						+ " | " + className);

		if (className != null) {
			// RESPONSE recv
			if (className.equals(ResponseInfo.class.getCanonicalName())) {
				ResponseInfo responseMessage = (ResponseInfo) Serialization
						.fromJavaByteStream(message.getContent());
				System.out.println("[RegistryCoreClient] Response type: "
						+ responseMessage.getType() + " | payload: "
						+ responseMessage.getPayload());
			} else {
				System.out
						.println("[RegistryCoreClient] Objeto desconhecido recebido do servidor: "
								+ className);
			}
		} else {
			System.out
					.println("[RegistryCoreClient] Objeto inválido recebido do servidor");
		}
	}

	// other methods

	@Override
	public void reconnected(NodeConnection remoteCon, SocketAddress endPoint,
			boolean wasHandover, boolean wasMandatory) {
		System.out.println("[RegistryCoreClient] Reconectou");
	}

	@Override
	public void disconnected(NodeConnection remoteCon) {
		System.out.println("[RegistryCoreClient] Desconectou");
	}

	@Override
	public void unsentMessages(NodeConnection remoteCon,
			List<Message> unsentMessages) {
	}

	@Override
	public void internalException(NodeConnection remoteCon, Exception e) {
	}
}
