package com.sieci;

import java.net.Socket;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Arrays;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.Pane;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;
import javafx.stage.WindowEvent;

import org.opencv.imgproc.Imgproc;
import org.opencv.core.Core;
import org.opencv.core.Mat;
import org.opencv.videoio.VideoCapture;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.core.MatOfInt;
import org.opencv.core.Size;
import org.opencv.core.MatOfByte;

public class MainApp extends Application {
	
	VideoCapture videoCapture;
	@Override
	public void start(Stage stage) throws Exception {
		
		stage.setTitle("Klient"); //okienko
		stage.setMinWidth(300);
		stage.setMinHeight(400);
		
		final Button doButton = new Button("Połącz");//tworzę przycisk Połącz
		doButton.setMinSize(120, 70);

		
		final Label ipLabel = new Label("Podaj adres IP serwera: ");
		final TextField ipField = new TextField();//okienko do wpisania adresu IP
		ipField.setMinWidth(200);
		
		
		final Label portLabel = new Label("Podaj nr portu serwera: ");
		final TextField portField = new TextField();//okienko do wpisania portu serwera
		portField.setMaxWidth(100);
		
		final Label cameraLabel = new Label("");
		final Label messageLabel = new Label("");
		
		
		GridPane.setConstraints(ipLabel, 1, 0);//tutaj w następnych linijkach inicjalizuję obiekty potrzebne do GUI
		GridPane.setConstraints(ipField, 1, 1);
		
		GridPane.setConstraints(portLabel, 1, 6);
		GridPane.setConstraints(portField, 1, 7);
		
		GridPane.setConstraints(doButton, 1, 12);
		
		GridPane.setConstraints(cameraLabel, 1, 17);
		GridPane.setConstraints(messageLabel, 1, 18);
		
		final GridPane inputGridPane = new GridPane();
		inputGridPane.setHgap(6);
		inputGridPane.setVgap(6);
		inputGridPane.getChildren().addAll(ipLabel,ipField,portLabel,portField,doButton,cameraLabel, messageLabel);

		final Pane rootGroup = new VBox(12);
		rootGroup.getChildren().addAll(inputGridPane);
		rootGroup.setPadding(new Insets(12, 12, 12, 12));

		stage.setScene(new Scene(rootGroup));
		stage.show();
		
				
		doButton.setOnAction(new EventHandler<ActionEvent>() {//po wciśnięciu przycisku wykonuje się "właściwa" część programu
           	public void handle(ActionEvent e) {
            	
            	
                if ( !(portField.getText().equals("")) && !(ipField.getText().equals("") )) {//jeśli pola IP i nr portu nie są puste
        		
            	    String ipAddress = ipField.getText();//pobieram adres IP i nr portu
            	    String portAddress = portField.getText();
	            		            		
                    System.loadLibrary(Core.NATIVE_LIBRARY_NAME);

                    videoCapture = new VideoCapture(); //otwieram kamerę wbudowaną w laptopie
                    videoCapture.open(0);
                    
                    if ( !videoCapture.isOpened() )  {
                    	cameraLabel.setText("Camera open failed");
                    }
                    else{
                    	cameraLabel.setText("Camera open succes");
                    }
                  
                    Thread t= new Thread(() -> {//tworzę wątek, który będzie obsługiwał połączenie i wysyłał obraz
                    	
                    int port= 0;
                    try {
            			port = Integer.parseInt(portAddress); //parsuję nr portu do integer
            		} catch(Exception e1) {
            		    Platform.runLater(()->messageLabel.setText("Port has to be a number"));
            		}
                    
                    try (Socket socket = new Socket(ipAddress, port);) { //tworzę gniazdo, które będzie obsługiwać połączenie z podanym adresem IP i nr portu
        	
                    	messageLabel.setText("Connection success");
                    	DataOutputStream output = new DataOutputStream(socket.getOutputStream()); //tworzę łącze do wysyłania danych
                    	 
                    	stage.setOnCloseRequest(new EventHandler<WindowEvent>() { //obsługa przycisku wyłączającego
                             public void handle(WindowEvent we) {
                            	 try {
                            		output.writeInt(55);//(-20); //wysyłam 55 do serwera, żeby wiedział, że zamykam program (a tym samym streaming)
					socket.close();
					System.out.println("Stage is closing");
					Thread.currentThread().interrupt();
					stage.close();
					return;
                           		  
				} catch (IOException e) {
					System.out.println("Error");
					e.printStackTrace();
				}
                              }
                         }); 
                    	
                    	while(true) {
                    		Mat send = new Mat(); //tworzę zmienną typu Mat do której zapiszę klatkę
                    		Mat frame = new Mat();
                    		
                    		boolean bSuccess = videoCapture.read(send); //pobieram klatkę z kamerki
                    		if (!bSuccess){
                    			System.out.println("Camera read failed");
                    			break;
                    		}
                    		//resize(send, frame, Size(320, 240), 0, 0, INTER_LINEAR);
                    		
                    		Size sz = new Size(320,240); //zmniejszam klatkę do rozmiarów 320x240
                    		Imgproc.resize( send, frame, sz );
                    		
                    		MatOfInt  params = new MatOfInt(Imgcodecs.IMWRITE_JPEG_QUALITY, 40); //ustalam parametry klatki, m.in jakość pogorszona do 40%, żeby wysłać mniej danych
                    		MatOfByte encoded = new MatOfByte();
                    		
                    		Imgcodecs.imencode(".jpg", frame, encoded, params); //kodowanie klatki do danych postaci MatOfByte 
                    		
                    		String size_wr = encoded.size().toString(); //uzyskuję w ten sposób rozmiar danych (klatki), które chcę wysłać do serwera
                    		int size = Integer.parseInt(size_wr.replace("1x",""));
                    		                   		
                    		int total_pack = 1 + (size-1)/ 1200; //obliczam w ilu paczkach o rozmiarze 1200 mogę wysłać mu klatkę 
                    		
                    		byte[] encoded_list = encoded.toArray(); //dane z klatki zapisane wcześniej do postaci MatOfByte wpisuje do tablicy typu byte
                    		
                    		output.writeInt(total_pack); //wysyłam do serwera w ilu paczkach zostanie wysłana jedna klatka
                    		
                   			//System.out.println(encoded_list.length);
                   			                   			
                   		for (int i = 0; i < total_pack; i++){ //wysyłanie klatki w postaci kilku paczek
                   			byte[] encoded_splitted = Arrays.copyOfRange(encoded_list,i*1200,(i+1)*1200);
                                	output.write(encoded_splitted);
                                                               
                                	try {
                        			Thread.sleep(500);                 //robię małe przerwy w wysyłaniu każdej paczki z danymi, żeby uniknąć błędów
                        		} catch(InterruptedException ex) {
                        		    	Thread.currentThread().interrupt();
                        		}                              
                            }                  		                   		                   			
                    	}                   	                  	
                    }
                    catch (IOException e1) {
                    	Platform.runLater(() ->messageLabel.setText("Connection failed"));
                    	
		}
                   
                    });
                                                          
                    t.setDaemon(true);
                    t.start();
                    
            	}
            	else 
            	{
            		messageLabel.setText("Incorrect IP address or port number");
            	}    
            }         
        });
		
				
	}


	public static void main(String[] args) {
		Application.launch(args);
	}


}
