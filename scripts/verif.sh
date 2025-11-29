#!/bin/bash

# Script (généré par IA) permettant de vérifier le respect des conventions pour le sokoban

#  ./fichier.sh <fichier.c>

if [ $# -eq 0 ]; then
    echo "Utilisation : $0 <fichier.c>"
    exit 1
fi

if [ ! -f "$1" ]; then
    echo "Erreur: Le fichier '$1' n'existe pas."
    exit 1
fi

fichier="$1"

# Compteurs d'erreurs
declare -i var_courtes=0
declare -i camelcase_err=0
declare -i const_err=0
declare -i func_err=0
declare -i espace_err=0
declare -i longueur_err=0
declare -i indent_err=0
declare -i lignes_func_err=0
declare -i erreur_count=0

# Variables pour le suivi des fonctions
declare -i in_function=0
declare -i function_lines=0
declare -i function_start=0
declare -i brace_count=0

echo "========================================"
echo "Vérification du fichier: $fichier"
echo "========================================"
echo

ligne_num=0

while IFS= read -r ligne || [ -n "$ligne" ]; do
    ((ligne_num++))
    
    # Ignorer les lignes vides et commentaires
    if [[ "$ligne" =~ ^[[:space:]]*$ ]] || [[ "$ligne" =~ ^[[:space:]]*// ]]; then
        continue
    fi
    
    # Vérification 1: Longueur de ligne (max 80 caractères)
    # Enlever les espaces/tabs de début (indentation)
    ligne_sans_indent="${ligne#"${ligne%%[![:space:]]*}"}"
    longueur=${#ligne_sans_indent}
    if [ $longueur -gt 80 ]; then
        echo "[LIGNE $ligne_num] Longueur excessive: $longueur caractères (max 80, hors indentation)"
        ((longueur_err++))
        ((erreur_count++))
    fi
    
    # Vérification 2: Indentation (max 5 niveaux = 20 espaces si 4 espaces/niveau)
    if [[ "$ligne" =~ ^([[:space:]]+) ]]; then
        spaces="${BASH_REMATCH[1]}"
        nb_spaces=${#spaces}
        indent_level=$((nb_spaces / 4))
        if [ $indent_level -gt 5 ]; then
            echo "[LIGNE $ligne_num] Indentation excessive: $indent_level niveaux (max 5)"
            ((indent_err++))
            ((erreur_count++))
        fi
    fi
    
    # Vérification 3: Variables courtes (une lettre sauf i, j, x, y)
    # Recherche de déclarations de variables
    if [[ "$ligne" =~ (int|char|float|double|long|short|unsigned)[[:space:]]+([a-hk-wz])[[:space:]\;=] ]]; then
        echo "[LIGNE $ligne_num] Variable courte détectée (une lettre, sauf i,j,x,y): ${BASH_REMATCH[2]}"
        ((var_courtes++))
        ((erreur_count++))
    fi
    
    # Vérification 4: Espaces dans les parenthèses des structures de contrôle
    # Correct: if (x > 0)  |  Incorrect: if ( x > 0) ou if (x > 0 )
    if [[ "$ligne" =~ (if|for|while|switch)[[:space:]]*\([[:space:]] ]]; then
        echo "[LIGNE $ligne_num] Espace après '(' dans structure de contrôle"
        ((espace_err++))
        ((erreur_count++))
    fi
    if [[ "$ligne" =~ (if|for|while|switch).*[[:space:]]\) ]]; then
        echo "[LIGNE $ligne_num] Espace avant ')' dans structure de contrôle"
        ((espace_err++))
        ((erreur_count++))
    fi
    
    # Suivi des fonctions pour compter les lignes
    if [[ "$ligne" =~ ^[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\( ]]; then
        if [[ "$ligne" =~ \{ ]]; then
            in_function=1
            function_start=$ligne_num
            function_lines=1
            brace_count=1
            
            # Vérification 5: Nom de fonction (snake_case)
            if [[ "$ligne" =~ ([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]]*\( ]]; then
                func_name="${BASH_REMATCH[1]}"
                # Vérifier si le nom contient des majuscules (pas snake_case)
                if [[ "$func_name" =~ [A-Z] ]]; then
                    echo "[LIGNE $ligne_num] Nom de fonction pas en snake_case: $func_name"
                    ((func_err++))
                    ((erreur_count++))
                fi
            fi
        fi
    elif [ $in_function -eq 1 ]; then
        ((function_lines++))
        
        # Compter les accolades pour détecter la fin de fonction
        while [[ "$ligne" =~ \{ ]]; do
            ((brace_count++))
            ligne="${ligne#*\{}"
        done
        ligne_temp="$ligne"
        while [[ "$ligne_temp" =~ \} ]]; do
            ((brace_count--))
            ligne_temp="${ligne_temp#*\}}"
        done
        
        if [ $brace_count -eq 0 ]; then
            if [ $function_lines -gt 60 ]; then
                echo "[LIGNES $function_start-$ligne_num] Fonction trop longue: $function_lines lignes (max 60)"
                ((lignes_func_err++))
                ((erreur_count++))
            fi
            in_function=0
            function_lines=0
        fi
    fi
    
    # Vérification 6: Constantes (#define en SNAKE_CASE majuscule)
    if [[ "$ligne" =~ ^[[:space:]]*#define[[:space:]]+([a-zA-Z_][a-zA-Z0-9_]*) ]]; then
        const_name="${BASH_REMATCH[1]}"
        if [[ "$const_name" =~ [a-z] ]]; then
            echo "[LIGNE $ligne_num] Constante pas en SNAKE_CASE majuscule: $const_name"
            ((const_err++))
            ((erreur_count++))
        fi
    fi
    
    # Vérification 7: Variables (camelCase - pas d'underscore)
    # Exclure les constantes (tout en majuscule avec underscore) et structures (t_...)
    if [[ "$ligne" =~ (int|char|float|double|long|short|unsigned)[[:space:]]+([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]\;=\[] ]]; then
        var_name="${BASH_REMATCH[2]}"
        # Ignorer si c'est une constante (tout en majuscule)
        if [[ "$var_name" =~ ^[A-Z_][A-Z0-9_]*$ ]]; then
            continue
        fi
        # Ignorer si c'est une structure (commence par t_)
        if [[ "$var_name" =~ ^t_ ]]; then
            continue
        fi
        # Vérifier si contient un underscore (pas camelCase)
        if [[ "$var_name" =~ _ ]]; then
            echo "[LIGNE $ligne_num] Variable pas en camelCase (underscore détecté): $var_name"
            ((camelcase_err++))
            ((erreur_count++))
        fi
    fi
    
done < "$fichier"

echo
echo "========================================"
echo "RÉSUMÉ DES ERREURS"
echo "========================================"
echo "Variables courtes:          $var_courtes"
echo "Erreurs camelCase:          $camelcase_err"
echo "Erreurs constantes:         $const_err"
echo "Erreurs nom fonction:       $func_err"
echo "Erreurs espaces:            $espace_err"
echo "Lignes trop longues:        $longueur_err"
echo "Indentations excessives:    $indent_err"
echo "Fonctions trop longues:     $lignes_func_err"
echo "========================================"
echo "TOTAL: $erreur_count erreur(s)"
echo "========================================"

exit 0