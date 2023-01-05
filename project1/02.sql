SELECT t.name
FROM trainer AS t, gym
WHERE t.name NOT IN (
		SELECT t.name
		FROM trainer AS t, gym
		WHERE t.id = gym.leader_id)
GROUP BY t.name
ORDER BY t.name;