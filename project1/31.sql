SELECT DISTINCT trainer.name
FROM (
        SELECT MAX(caughtpokemon.level)
        FROM caughtpokemon) AS maxlevel(level),
     (
        SELECT MIN(caughtpokemon.level)
        FROM caughtpokemon) AS minlevel(level),
    trainer
JOIN caughtpokemon ON caughtpokemon.owner_id = trainer.id
WHERE caughtpokemon.level = maxlevel.level OR caughtpokemon.level = minlevel.level
ORDER BY trainer.name;