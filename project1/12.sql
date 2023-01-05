SELECT DISTINCT trainer.hometown, caughtpokemon.nickname
FROM trainer, caughtpokemon, (
		SELECT trainer.hometown, MAX(caughtpokemon.level)
		FROM trainer, caughtpokemon
		WHERE trainer.id = caughtpokemon.owner_id
		GROUP BY trainer.hometown) AS hometownpokemon(hometown, level)
WHERE trainer.id = caughtpokemon.owner_id
AND caughtpokemon.level = hometownpokemon.level
AND hometownpokemon.hometown = trainer.hometown
ORDER BY trainer.hometown;