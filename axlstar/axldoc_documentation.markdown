Axldoc evolution   {#mainpage}
================

Description des �volutions pour axldoc (SDC)
-------------------------------------

Test au passage de l'utilisation du format markdown pour int�gration dans doxygen. 

Les nouvelles fonctionalit�s sont les suivantes:

- Ajout d'un outillage pour la g�n�ration d'un sch�ma xsd de l'application :
    - g�n�re un xsd avec une collectino de type pour les services : l'association entre une balise de service et  son type n'est pas automatique � la validation et doit �tre pr�cis� dans le .arc, balise xsi:type  dans balise service), 
(semble difficile � faire automatiquement, cf. types de services non discernables). Ex :  
      <pre>&ltformat name="IXM4Writer" xsi:type="Arcane-IPostProcessorWriter_IXM4Writer-type"&gt</pre>
      @IFPEN nous avons script� l'ajout de l'info dans le xsd pour permettre la validation, ie ajout du type xsi:type dans les balises services d'un .arc existant.
    
    - Comme le xsd ne permet pas de d�finir un ordre al�atoire dans les options (en 1.0) , par d�faut il impose l'ordre alphab�tique dans les balises. P
Pour pouvoir imposer un ordre diff�rent on peut utiliser une balise <option-index> dans la balise <description> des axl (todo pouvoir conserver l'ordre dans lequel sont d�finies les options)
    - Au niveau code, ajout des classes XmlSchemaFile et XmlSchemaVisitor.
    - Le branchement s'effectue par l'option --generate-xsd (l'ajout d'info dans le xsd, cf les options possibles est utilis� @IFPEN pour ajouter dans des .arc existant l'info de type dans les balises services et pouvoir ainsi faire de la validation).
		
- Ajout de lien retour d'un service vers les modules qui l'utilisent (qui d�clarent son interface en option)

- Ces diff�rentes modifs ont n�cessit� l'enrichissement de l'axl\_db_file (todo : faire un sch�ma xsd ?)
	  - ajout attribut application-name dans la balise root (pour le branchement applicatif)
	  - balise has-service-instance : pour lien retour (depuis une page de service, on voit toutes les occurrence de l'utilisation de son interface)
	  - balise alias : pour la g�n�ration du xsd (parfois les services ont plusieurs noms et �a pose probl�me pour valider) (=> donner un ex)


- Ajout de la possibilit� de g�n�rer des pages propres aux applications via un branchement plugin applicatif (interface IPrivateApplicationPages)

- Modification cosm�tique : s�parateur entre option (pour mieux s�parer les options complexes et les autres) et une intro avant la table des services
